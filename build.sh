cp -R res/ build/linux/x86_64/release/
xmake project -k cmakelists
xmake
xmake run