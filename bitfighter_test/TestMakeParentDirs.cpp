#include <gtest/gtest.h>

#include <string>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#  include <windows.h>
#  include <direct.h>
#  include <io.h>
#  define MKDIR(p) _mkdir(p)
#  define RMDIR _rmdir
#else
#  include <unistd.h>
#  include <dirent.h>
#  include <errno.h>
#  define MKDIR(p) mkdir((p), 0755)
#  define RMDIR rmdir
#endif

namespace TNL {
    // Declaration for the function under test (defined in bitfighter\tnl\log.cpp)
    bool makeParentDirs(const std::string &filepath);
}

namespace Zap
{

// Helper: check path exists and is directory
static bool isDirectory(const std::string &path)
{
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) return false;
    return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return false;
    return S_ISDIR(st.st_mode);
#endif
}

// Recursive cleanup for small test trees (keeps test portable and simple)
static void removeDirectoryRecursive(const std::string &path)
{
#ifdef _WIN32
    WIN32_FIND_DATAA fd;
    std::string pattern = path + "\\*";
    HANDLE h = FindFirstFileA(pattern.c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE)
    {
        do {
            std::string name = fd.cFileName;
            if (name == "." || name == "..") continue;
            std::string child = path + "\\" + name;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                removeDirectoryRecursive(child);
            else
                DeleteFileA(child.c_str());
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
    RemoveDirectoryA(path.c_str());
#else
    DIR *d = opendir(path.c_str());
    if (!d)
    {
        // not a directory or doesn't exist: try unlink
        unlink(path.c_str());
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL)
    {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;
        std::string child = path + "/" + name;
        struct stat st;
        if (stat(child.c_str(), &st) == 0)
        {
            if (S_ISDIR(st.st_mode))
                removeDirectoryRecursive(child);
            else
                unlink(child.c_str());
        }
    }
    closedir(d);
    rmdir(path.c_str());
#endif
}


TEST(MakeParentDirsTest, NoParentPathReturnsTrue)
{
    std::string file = "simple.log"; // no directory component
    EXPECT_TRUE(TNL::makeParentDirs(file));
}


TEST(MakeParentDirsTest, CreatesNestedDirectories)
{
    std::string base = "test_make_parent_dirs_tmp";
    std::string nested = base + "/a/b/c";
    std::string file = nested + "/log.txt";

    // Ensure clean start
    removeDirectoryRecursive(base);

    EXPECT_TRUE(TNL::makeParentDirs(file));

    EXPECT_TRUE(isDirectory(base));
    EXPECT_TRUE(isDirectory(base + "/a"));
    EXPECT_TRUE(isDirectory(base + "/a/b"));
    EXPECT_TRUE(isDirectory(nested));

    // Idempotency
    EXPECT_TRUE(TNL::makeParentDirs(file));

    // Cleanup
    removeDirectoryRecursive(base);
    EXPECT_FALSE(isDirectory(base));
}


TEST(MakeParentDirsTest, ExistingDirectorySucceeds)
{
    std::string base = "test_existing_dir";
    std::string sub = base + "/sub";
    std::string file = sub + "/log.txt";

    removeDirectoryRecursive(base);

    ASSERT_EQ(0, MKDIR(base.c_str()));
    ASSERT_EQ(0, MKDIR(sub.c_str()));

    EXPECT_TRUE(TNL::makeParentDirs(file));

    removeDirectoryRecursive(base);
    EXPECT_FALSE(isDirectory(base));
}


//int main(int argc, char **argv)
//{
//    ::testing::InitGoogleTest(&argc, argv);
//    return RUN_ALL_TESTS();
//}

}