# Anbernic RG35XX H Test program

For Knulli OS at the moment, a testprogram to learn making programs for the Anbernic RG35XX H.
Currently using the Knulli sysroot but not any docker. I dont really like to use docker.

Support for stock os might also be cool.

## Building

Tricky, basicly I got the knulli toolchain and instead of doing this the docker way, we extract and change PATH and point everything to it to compile our program, seems to work. might be dangerous. Here be hacker dragons.

```sh
wget -c -O aarch64-buildroot-linux-gnu_sdk-buildroot.tar.gz https://github.com/knulli-cfw/toolchains/releases/download/rg35xx-plush-sdk-20240421/aarch64-buildroot-linux-gnu_sdk-buildroot.tar.gz

tar -xzf aarch64-buildroot-linux-gnu_sdk-buildroot.tar.gz
```

then go set some variables:
```sh
export PATH=$PWD/aarch64-buildroot-linux-gnu_sdk-buildroot/bin:$PATH
export SYSROOT=$PWD/aarch64-buildroot-linux-gnu_sdk-buildroot/aarch64-buildroot-linux-gnu/sysroot
alias aarch64-gcc="aarch64-buildroot-linux-gnu-gcc --sysroot=$SYSROOT"

export CC="aarch64-buildroot-linux-gnu-gcc"
export CXX="aarch64-buildroot-linux-gnu-g++"
export AR="aarch64-buildroot-linux-gnu-ar"
export LD="aarch64-buildroot-linux-gnu-ld"
export STRIP="aarch64-buildroot-linux-gnu-strip"
export PKG_CONFIG_PATH="$SYSROOT/usr/lib/pkgconfig"
export PKG_CONFIG_SYSROOT_DIR="$SYSROOT"
```
Yes, I use a script for this, might publish, need to research if any of the above is actually needed.

run this to build:
```sh
make CC=$CC CXX=$CXX AR=$AR LD=$LD STRIP=$STRIP PKG_CONFIG_PATH=$PKG_CONFIG_PATH PKG_CONFIG_SYSROOT_DIR=$PKG_CONFIG_SYSROOT_DIR VERBOSE=1
```


## Running

Just run the program, quit by pressing and releasing button 11/16 (F) on the Anbernic RG35XX H.

## Todo

- Replace the original m8c square box with anything else
- Make whatever object will be there, react to the joystick input
- color, textures
- Audio 
- Replace or get permision for the included fonts.

## Thanks

- [laamaa](https://github.com/laamaa) - Jonne Kokkonen for your m8c code I butchered and basicly took as a SDL2 template.
- [jamesMcMeex](https://github.com/jamesMcMeex) - James, Thanks for your H700 fork, which I adapted for this project. Your code worked seamlessly on both the stock OS and Knulli OS.  
- [knulli](https://knulli.org/) - Thanks team behind Knulli, for your amazing handheld distro, which is based on Batocera. It makes the handheld so much cleaner to run and play games on. And lets not forget about the toolchain you folks publicly share too.
- Trash80 for their awesome music, and apparently font. Hope for now its ok I kept the font in the sources.
- And any other people in my life who support me, or find me a bit annoying. Thanks for sticking with me and replying to my silly questions or messages.

## License

This project is licensed under the MIT License. See the `LICENSE` file for more details.