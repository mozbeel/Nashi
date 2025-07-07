{
  pkgs ? import <nixpkgs> { },
}:


pkgs.mkShell rec {
  buildInputs = with pkgs; [
    sdl3
    cmake
    pkg-config

    wayland
    wayland-protocols
    egl-wayland
    mesa

    libgbm
    glm

    vulkan-headers
    vulkan-tools
    vulkan-loader

    wayland-utils
    egl-wayland
    xorg.libX11
    libgbm
    alsa-lib
    jack2
    pipewire
    sndio
    libxkbcommon
    libunwind
    pulseaudio
    libusb1

    libGL
    libdrm
    dbus
    ibus
    liburing

    # zlib
    # stdenv.cc.cc
    # git
    # gitRepo
    # gnupg
    # autoconf
    # curl
    # procps
    # gnumake
    # util-linux
    # m4
    # gperf
    # libGLU
    # libGL
    # ncurses5
    # stdenv.cc
    # binutils
    # # Wayland and related libraries
    # wayland-protocols
    # wayland
    # waylandpp
    # egl-wayland
    # # Missing libraries
    # jack2
    # pipewire
    # sndio
    # libdrm
    # libgbm
    # dbus
    # ibus
    # # liburing-ffi
    # libunwind
    # libusb
  ];

  shellHook = ''
    export LD_LIBRARY_PATH=${pkgs.lib.makeLibraryPath buildInputs}
  '';
}

