#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define NOVACORE_PLATFORM_WINDOWS 1
#else
#define NOVACORE_PLATFORM_WINDOWS 0
#endif

static int RunCommand(const char* command) {
    printf("> %s\n", command);
    fflush(stdout);
    int result = system(command);
    return result;
}

static void PrintUsage(const char* programName) {
    printf("Usage: %s <target> [config]\n\n", programName);
    printf("Targets:\n");
    printf("  windows       Configure and build the Windows desktop target\n");
    printf("  android       Configure and build the Android arm64-v8a target\n");
    printf("  clean         Remove the build directory\n\n");
    printf("Config (optional, defaults to Debug):\n");
    printf("  Debug\n");
    printf("  Release\n\n");
    printf("Examples:\n");
    printf("  %s windows\n", programName);
    printf("  %s windows Release\n", programName);
    printf("  %s android\n", programName);
    printf("  %s clean\n", programName);
}

static int BuildWindows(const char* config) {
    char configureCommand[1024];
    char buildCommand[512];

    const char* vcpkgRoot = getenv("VCPKG_ROOT");
    if (!vcpkgRoot) {
        vcpkgRoot = NOVACORE_PLATFORM_WINDOWS ? "C:\\vcpkg" : "~/vcpkg";
    }

    snprintf(configureCommand, sizeof(configureCommand),
        "cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=\"%s/scripts/buildsystems/vcpkg.cmake\" -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_WARN_UNUSED_CLI=OFF -DCMAKE_BUILD_TYPE=%s -A x64",
        vcpkgRoot, config);

    if (RunCommand(configureCommand) != 0) {
        fprintf(stderr, "Configure step failed\n");
        return 1;
    }

    snprintf(buildCommand, sizeof(buildCommand),
        "cmake --build build --config %s --parallel", config);

    if (RunCommand(buildCommand) != 0) {
        fprintf(stderr, "Build step failed\n");
        return 1;
    }

    printf("\nBuild complete: build/%s/NovaCore.exe\n", config);
    return 0;
}

static int BuildAndroid(const char* config) {
    char configureCommand[1536];
    char buildCommand[512];

    const char* vcpkgRoot = getenv("VCPKG_ROOT");
    const char* ndkHome = getenv("ANDROID_NDK_HOME");

    if (!vcpkgRoot) {
        fprintf(stderr, "VCPKG_ROOT environment variable is not set\n");
        return 1;
    }

    if (!ndkHome) {
        fprintf(stderr, "ANDROID_NDK_HOME environment variable is not set\n");
        return 1;
    }

    snprintf(configureCommand, sizeof(configureCommand),
        "cmake -B build -S . "
        "-DCMAKE_TOOLCHAIN_FILE=\"%s/scripts/buildsystems/vcpkg.cmake\" "
        "-DVCPKG_TARGET_TRIPLET=arm64-android "
        "-DVCPKG_OVERLAY_TRIPLETS=\"triplets\" "
        "-DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=\"%s/build/cmake/android.toolchain.cmake\" "
        "-DANDROID_ABI=arm64-v8a "
        "-DANDROID_PLATFORM=android-24 "
        "-DCMAKE_WARN_UNUSED_CLI=OFF "
        "-DCMAKE_BUILD_TYPE=%s",
        vcpkgRoot, ndkHome, config);

    if (RunCommand(configureCommand) != 0) {
        fprintf(stderr, "Configure step failed\n");
        return 1;
    }

    snprintf(buildCommand, sizeof(buildCommand),
        "cmake --build build --config %s --parallel", config);

    if (RunCommand(buildCommand) != 0) {
        fprintf(stderr, "Build step failed\n");
        return 1;
    }

    printf("\nBuild complete: build/libmain.so\n");
    return 0;
}

static int CleanBuild(void) {
#if NOVACORE_PLATFORM_WINDOWS
    return RunCommand("rmdir /s /q build");
#else
    return RunCommand("rm -rf build");
#endif
}

int main(int argc, char** argv) {
    if (argc < 2) {
        PrintUsage(argv[0]);
        return 1;
    }

    const char* target = argv[1];
    const char* config = (argc >= 3) ? argv[2] : "Debug";

    if (strcmp(config, "Debug") != 0 && strcmp(config, "Release") != 0) {
        fprintf(stderr, "Invalid config: %s (expected Debug or Release)\n", config);
        return 1;
    }

    if (strcmp(target, "windows") == 0) {
        return BuildWindows(config);
    }

    if (strcmp(target, "android") == 0) {
        return BuildAndroid(config);
    }

    if (strcmp(target, "clean") == 0) {
        return CleanBuild();
    }

    fprintf(stderr, "Unknown target: %s\n\n", target);
    PrintUsage(argv[0]);
    return 1;
}
