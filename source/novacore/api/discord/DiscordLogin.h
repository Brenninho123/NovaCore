#pragma once

#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>

enum class DiscordLoginState {
    Idle,
    WaitingForRedirect,
    ExchangingToken,
    FetchingUser,
    LoggedIn,
    Failed,
    Cancelled
};

struct DiscordUser {
    std::string id;
    std::string username;
    std::string discriminator;
    std::string avatarHash;
};

class DiscordLogin {
public:
    static void Init(const std::string& clientId, int callbackPort = 47115);
    static void Shutdown();

    static void StartLogin(const std::vector<std::string>& scopes = { "identify" });
    static void Cancel();

    static DiscordLoginState GetState();
    static bool IsLoggedIn();

    static const DiscordUser& GetUser();
    static std::string GetAccessToken();
    static std::string GetAvatarUrl();

    static std::string GetLastError();

private:
    static void RunRedirectServer(std::string expectedState);
    static void ExchangeToken(const std::string& code);
    static void FetchUser();

    static std::string GenerateRandomString(size_t length);
    static std::string Sha256Base64Url(const std::string& input);
    static std::string BuildAuthorizeUrl(const std::vector<std::string>& scopes, const std::string& state, const std::string& codeChallenge);

    static std::string clientId;
    static std::string redirectUri;
    static int callbackPort;

    static std::string codeVerifier;

    static std::atomic<DiscordLoginState> state;
    static std::thread workerThread;

    static std::mutex dataMutex;
    static DiscordUser currentUser;
    static std::string accessToken;
    static std::string lastError;
};
