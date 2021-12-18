cp -R res/ build/linux/x86_64/release/
xmake project -k cmakelists
xmake
prime-run xmake run