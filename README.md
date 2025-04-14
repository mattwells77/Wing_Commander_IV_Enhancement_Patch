# Wing Commander IV Enhancement Patch
This patch was created for the Windows version of the PC game "Wing Commander IV".

This patch uses DirectX 11 for rendering and will likely require Windows 10 or greater to use.

This patch makes use of [DLL injection](https://en.wikipedia.org/wiki/DLL_injection) to make changes to the game's executable file. And is loaded by the game itself by pretending to be the Direct Draw library file "ddraw.dll". It is thus incompatible with patches which use the same method to load eg. [DirectDraw_Hack](https://www.wcnews.com/wcpedia/DirectDraw_Hack "DirectDraw DLL replacement").


HD Movie playback is achieved using [libvlc](https://www.videolan.org/vlc/libvlc.html "libVLC is the core engine and the interface to the multimedia framework on which VLC media player is based.").
## Current list of enhancements:
- HD Space, space scenes are displayed at your desktop resolution(formerly 640x480).
- Support for the [Wing Commander 4 HD Video Pack](https://www.wcnews.com/wcpedia/Wing_Commander_4_HD_Video_Pack) created by ODVS. 
- Improved windowed mode, with window resizing options and mouse locking only when required in-game.
- Ingame Joystick\Controller configuration utility.

## Installation:
- Click on [Releases](https://github.com/mattwells77/Wing_Commander_IV_Enhancement_Patch/releases) and download the latest "wc4w_en_x.x.x.zip" where "x" is the version number and "libvlc_min_pack_3.0.20.zip" files. Extract the contents of these files to your Wing Commander 4 Install directory.
- For optional HD movie playback: Download the [Wing Commander 4 HD Video Pack](https://www.wcnews.com/wcpedia/Wing_Commander_4_HD_Video_Pack) created by ODVS and hosted by the [Wing Commander CIC](https://www.wcnews.com/#). Extract the movies to the "vobs" folder in your Wing Commander 4 Install directory.
- Once Installed, optional settings can be edited in the "wc4w_en.ini" file. Further details can be found within the file.

## Compiling:
- To compile, this project requires the [VLC media player sdk vlc-3.0.21](https://download.videolan.org/pub/videolan/vlc/last/win32/) and the [libvlcpp C++ bindings](https://github.com/videolan/libvlcpp). I've set up relative paths for these, and they should be installed adjacent to the projects solution folder to avoid the need to modify any project settings.
- There is also an included batch file "post_build_copy_to_game_folder.cmd". That is set to copy the newly created DLL to your game folder after the build process completes. This should be edited to match your game installation path.
