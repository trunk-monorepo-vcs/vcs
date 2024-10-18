# Client for VCS

There is (in progress) client on FUSE proxyfs

client is built using Meson (http://mesonbuild.com).

    meson build
    ninja -C build install

How it works:
    0. Program's excecution starts at some dir
    1. In current dir creates sub_dir 'vcs/'
    2. It mounts FUSE proxy to vcs/
    