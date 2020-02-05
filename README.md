# Media Process Platform (MPP) for Android Container

Based on [rockchip-linux/mpp](https://github.com/rockchip-linux/mpp) distribution **mpp-20190904_1.00**, see [readme](./readme.txt)

## Build for Android

1. Environment **ANDROID_NDK_HOME** is set

2. Run _build_android.sh_ and output to **build-${ANDROID_ABI}** folder

    ```Bash
    # usage: build-android.sh $ANDROID_ABI
    #    where ANDROID_ABI is {armeabi-v7a|arm64-v8a|x86|x86_64}
    ./build_android.sh x86_64
    ```

## Build for Linux

1. Run _build_linux.sh_

    ```Bash
    ./build_linux.sh
    ```
