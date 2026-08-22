#include "DiscordLogin.h"

#include <SDL2/SDL.h>
#include <curl/curl.h>
#include <openssl/sha.h>
#include <nlohmann/json.hpp>

#include <random>
#include <sstream>
#include <iomanip>
#include <cstring>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

using json = nlohmann::json;

std::string DiscordLogin::clientId;
std::string DiscordLogin::redirectUri;
int DiscordLogin::callbackPort = 47115;
std::string DiscordLogin::codeVerifier;

std::atomic<DiscordLoginState> DiscordLogin::state{ DiscordLoginState::Idle };
std::thread DiscordLogin::workerThread;

std::mutex DiscordLogin::dataMutex;
DiscordUser DiscordLogin::currentUser;
std::string DiscordLogin::accessToken;
std::string DiscordLogin::lastError;

static size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* buffer = static_cast<std::string*>(userp);
    buffer->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

void DiscordLogin::Init(const std::string& id, int port) {
    clientId = id;
    callbackPort = port;
    redirectUri = "http://127.0.0.1:" + std::to_string(port) + "/callback";

    curl_global_init(CURL_GLOBAL_DEFAULT);

#if defined(_WIN32)
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

void DiscordLogin::Shutdown() {
    Cancel();

    if (workerThread.joinable()) {
        workerThread.join();
    }

    curl_global_cleanup();

#if defined(_WIN32)
    WSACleanup();
#endif
}

std::string DiscordLogin::GenerateRandomString(size_t length) {
    static const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<size_t> distribution(0, sizeof(charset) - 2);

    std::string result;
    result.reserve(length);

    for (size_t i = 0; i < length; i++) {
        result += charset[distribution(generator)];
    }

    return result;
}

std::string DiscordLogin::Sha256Base64Url(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);

    static const char encodeTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string base64;
    int i = 0;
    unsigned char array3[3];
    unsigned char array4[4];
    size_t inputLength = SHA256_DIGEST_LENGTH;
    const unsigned char* data = hash;

    while (inputLength--) {
        array3[i++] = *(data++);
        if (i == 3) {
            array4[0] = (array3[0] & 0xfc) >> 2;
            array4[1] = ((array3[0] & 0x03) << 4) + ((array3[1] & 0xf0) >> 4);
            array4[2] = ((array3[1] & 0x0f) << 2) + ((array3[2] & 0xc0) >> 6);
            array4[3] = array3[2] & 0x3f;

            for (i = 0; i < 4; i++) {
                base64 += encodeTable[array4[i]];
            }
            i = 0;
        }
    }

    if (i) {
        for (int j = i; j < 3; j++) {
            array3[j] = '\0';
        }

        array4[0] = (array3[0] & 0xfc) >> 2;
        array4[1] = ((array3[0] & 0x03) << 4) + ((array3[1] & 0xf0) >> 4);
        array4[2] = ((array3[1] & 0x0f) << 2) + ((array3[2] & 0xc0) >> 6);

        for (int j = 0; j < i + 1; j++) {
            base64 += encodeTable[array4[j]];
        }
    }

    std::string urlSafe;
    for (char c : base64) {
        if (c == '+') urlSafe += '-';
        else if (c == '/') urlSafe += '_';
        else if (c == '=') continue;
        else urlSafe += c;
    }

    return urlSafe;
}

std::string DiscordLogin::BuildAuthorizeUrl(const std::vector<std::string>& scopes, const std::string& state_, const std::string& codeChallenge) {
    std::string scopeString;
    for (size_t i = 0; i < scopes.size(); i++) {
        scopeString += scopes[i];
        if (i + 1 < scopes.size()) {
            scopeString += "%20";
        }
    }

    std::ostringstream url;
    url << "https://discord.com/api/oauth2/authorize";
    url << "?client_id=" << clientId;
    url << "&redirect_uri=" << redirectUri;
    url << "&response_type=code";
    url << "&scope=" << scopeString;
    url << "&state=" << state_;
    url << "&code_challenge=" << codeChallenge;
    url << "&code_challenge_method=S256";
    url << "&prompt=consent";

    return url.str();
}

void DiscordLogin::StartLogin(const std::vector<std::string>& scopes) {
    if (state == DiscordLoginState::WaitingForRedirect ||
        state == DiscordLoginState::ExchangingToken ||
        state == DiscordLoginState::FetchingUser) {
        return;
    }

    if (workerThread.joinable()) {
        workerThread.join();
    }

    codeVerifier = GenerateRandomString(64);
    std::string codeChallenge = Sha256Base64Url(codeVerifier);
    std::string stateToken = GenerateRandomString(24);

    std::string authorizeUrl = BuildAuthorizeUrl(scopes, stateToken, codeChallenge);

    state = DiscordLoginState::WaitingForRedirect;

    {
        std::lock_guard<std::mutex> lock(dataMutex);
        lastError.clear();
    }

    SDL_OpenURL(authorizeUrl.c_str());

    workerThread = std::thread(&DiscordLogin::RunRedirectServer, stateToken);
}

void DiscordLogin::Cancel() {
    if (state == DiscordLoginState::WaitingForRedirect) {
        state = DiscordLoginState::Cancelled;
    }
}

void DiscordLogin::RunRedirectServer(std::string expectedState) {
#if defined(_WIN32)
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::lock_guard<std::mutex> lock(dataMutex);
        lastError = "Failed to create socket";
        state = DiscordLoginState::Failed;
        return;
    }
#else
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) {
        std::lock_guard<std::mutex> lock(dataMutex);
        lastError = "Failed to create socket";
        state = DiscordLoginState::Failed;
        return;
    }
#endif

    int reuse = 1;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons(static_cast<uint16_t>(callbackPort));

    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        std::lock_guard<std::mutex> lock(dataMutex);
        lastError = "Failed to bind local server";
        state = DiscordLoginState::Failed;
#if defined(_WIN32)
        closesocket(listenSocket);
#else
        close(listenSocket);
#endif
        return;
    }

    listen(listenSocket, 1);

#if defined(_WIN32)
    u_long mode = 1;
    ioctlsocket(listenSocket, FIONBIO, &mode);
#else
    fcntl(listenSocket, F_SETFL, O_NONBLOCK);
#endif

    std::string code;
    bool receivedCode = false;
    Uint32 startTicks = SDL_GetTicks();
    const Uint32 timeoutMs = 120000;

    while (state == DiscordLoginState::WaitingForRedirect) {
        if (SDL_GetTicks() - startTicks > timeoutMs) {
            std::lock_guard<std::mutex> lock(dataMutex);
            lastError = "Login timed out";
            state = DiscordLoginState::Failed;
            break;
        }

#if defined(_WIN32)
        SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
        bool valid = (clientSocket != INVALID_SOCKET);
#else
        int clientSocket = accept(listenSocket, nullptr, nullptr);
        bool valid = (clientSocket >= 0);
#endif

        if (!valid) {
            SDL_Delay(100);
            continue;
        }

        char buffer[4096];
        std::memset(buffer, 0, sizeof(buffer));

#if defined(_WIN32)
        recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
#else
        recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
#endif

        std::string request(buffer);
        size_t codePos = request.find("code=");

        std::string responseBody;

        if (codePos != std::string::npos) {
            size_t codeStart = codePos + 5;
            size_t codeEnd = request.find_first_of("& \r\n", codeStart);
            code = request.substr(codeStart, codeEnd - codeStart);
            receivedCode = true;
            responseBody = "<html><body>Login complete. You can close this window.</body></html>";
        } else {
            responseBody = "<html><body>Login failed or was cancelled.</body></html>";
        }

        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: text/html\r\n";
        response << "Content-Length: " << responseBody.size() << "\r\n";
        response << "Connection: close\r\n\r\n";
        response << responseBody;

        std::string responseStr = response.str();
        send(clientSocket, responseStr.c_str(), static_cast<int>(responseStr.size()), 0);

#if defined(_WIN32)
        closesocket(clientSocket);
#else
        close(clientSocket);
#endif

        break;
    }

#if defined(_WIN32)
    closesocket(listenSocket);
#else
    close(listenSocket);
#endif

    if (state == DiscordLoginState::Cancelled) {
        return;
    }

    if (!receivedCode) {
        if (state != DiscordLoginState::Failed) {
            std::lock_guard<std::mutex> lock(dataMutex);
            lastError = "No authorization code received";
            state = DiscordLoginState::Failed;
        }
        return;
    }

    ExchangeToken(code);
}

void DiscordLogin::ExchangeToken(const std::string& code) {
    state = DiscordLoginState::ExchangingToken;

    CURL* curl = curl_easy_init();
    if (!curl) {
        std::lock_guard<std::mutex> lock(dataMutex);
        lastError = "Failed to init curl";
        state = DiscordLoginState::Failed;
        return;
    }

    std::ostringstream postFields;
    postFields << "client_id=" << clientId;
    postFields << "&grant_type=authorization_code";
    postFields << "&code=" << code;
    postFields << "&redirect_uri=" << redirectUri;
    postFields << "&code_verifier=" << codeVerifier;

    std::string responseBuffer;
    std::string postFieldsStr = postFields.str();

    curl_easy_setopt(curl, CURLOPT_URL, "https://discord.com/api/oauth2/token");
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFieldsStr.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

    CURLcode result = curl_easy_perform(curl);

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_easy_cleanup(curl);

    if (result != CURLE_OK || httpCode != 200) {
        std::lock_guard<std::mutex> lock(dataMutex);
        lastError = "Token exchange failed";
        state = DiscordLoginState::Failed;
        return;
    }

    try {
        json parsed = json::parse(responseBuffer);
        std::string token = parsed.at("access_token").get<std::string>();

        {
            std::lock_guard<std::mutex> lock(dataMutex);
            accessToken = token;
        }
    } catch (...) {
        std::lock_guard<std::mutex> lock(dataMutex);
        lastError = "Failed to parse token response";
        state = DiscordLoginState::Failed;
        return;
    }

    FetchUser();
}

void DiscordLogin::FetchUser() {
    state = DiscordLoginState::FetchingUser;

    std::string token;
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        token = accessToken;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        std::lock_guard<std::mutex> lock(dataMutex);
        lastError = "Failed to init curl";
        state = DiscordLoginState::Failed;
        return;
    }

    std::string authHeader = "Authorization: Bearer " + token;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, authHeader.c_str());

    std::string responseBuffer;

    curl_easy_setopt(curl, CURLOPT_URL, "https://discord.com/api/users/@me");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

    CURLcode result = curl_easy_perform(curl);

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (result != CURLE_OK || httpCode != 200) {
        std::lock_guard<std::mutex> lock(dataMutex);
        lastError = "Failed to fetch user info";
        state = DiscordLoginState::Failed;
        return;
    }

    try {
        json parsed = json::parse(responseBuffer);

        DiscordUser user;
        user.id = parsed.value("id", "");
        user.username = parsed.value("username", "");
        user.discriminator = parsed.value("discriminator", "0");
        user.avatarHash = parsed.value("avatar", "");

        {
            std::lock_guard<std::mutex> lock(dataMutex);
            currentUser = user;
        }

        state = DiscordLoginState::LoggedIn;
    } catch (...) {
        std::lock_guard<std::mutex> lock(dataMutex);
        lastError = "Failed to parse user response";
        state = DiscordLoginState::Failed;
    }
}

DiscordLoginState DiscordLogin::GetState() {
    return state;
}

bool DiscordLogin::IsLoggedIn() {
    return state == DiscordLoginState::LoggedIn;
}

const DiscordUser& DiscordLogin::GetUser() {
    return currentUser;
}

std::string DiscordLogin::GetAccessToken() {
    std::lock_guard<std::mutex> lock(dataMutex);
    return accessToken;
}

std::string DiscordLogin::GetAvatarUrl() {
    std::lock_guard<std::mutex> lock(dataMutex);

    if (currentUser.avatarHash.empty()) {
        return "";
    }

    return "https://cdn.discordapp.com/avatars/" + currentUser.id + "/" + currentUser.avatarHash + ".png";
}

std::string DiscordLogin::GetLastError() {
    std::lock_guard<std::mutex> lock(dataMutex);
    return lastError;
}
