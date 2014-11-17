#include <git2/common.h>
#include <git2/version.h>

int main() {
#if LIBGIT2_VER_MAJOR <= 0 && LIBGIT2_VER_MINOR <= 20
    return !(git_libgit2_capabilities() & GIT_CAP_THREADS);
#else
    return !(git_libgit2_features() & GIT_FEATURE_THREADS);
#endif
}
