#include "BitfighterTestEnvironment.h"
#include "DisplayManager.h"
#include "TestRenderer.h"
#include "FontManager.h"
#include "Renderer.h"
// #include "ResourceChecker.h"   // whatever defines checkResources()

using namespace Zap;


// Helper function to check if a file exists  -- warning, repeated code
static bool fileExists(const std::string &filepath)
{
    struct stat buffer;
    return (stat(filepath.c_str(), &buffer) == 0);
}


// Returns true if all resource folders are in place, false otherwise
bool checkResources()
{
    const string dirs[] = { "editor_plugins", "fonts", "levels", "music", "robots", "scripts", "sfx", "testing" };

    for(S32 i = 0; i < ARRAYSIZE(dirs); ++i)
        if(!fileExists(dirs[i]))
            return false;

    return true;
}


void BitfighterTestEnvironment::SetUp() {
    if (!checkResources()) {
        printf("FAILED: Invalid test environment! Are you sure you copied everything from 'resources/' into 'exe/'?\n");
        testing::internal::posix::Abort();
    }

    DisplayManager::initialize();
    TestRenderer::install();
}

void BitfighterTestEnvironment::TearDown() {
    FontManager::cleanup();
    Renderer::shutdown();
    DisplayManager::cleanup();
}
