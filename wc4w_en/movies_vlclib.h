/*
The MIT License (MIT)
Copyright � 2025 Matt Wells

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the �Software�), to deal in the
Software without restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED �AS IS�, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "Display_DX11.h"
#include "vlcpp/vlc.hpp"


extern VLC::Instance vlc_instance;

//BOOL Create_Movie_Path(const char* mve_path, int branch, std::string* p_retPath);


struct MOVIE_STATE {
    LONG branch;
    LONG list_num;
    bool isPlaying;
    bool hasPlayed;
    bool isError;
};


class LibVlc_Movie {
public:
    LibVlc_Movie(std::string mve_path, LONG* branch_list, LONG branch_list_num);

    ~LibVlc_Movie() {
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
        is_stop_set = true;
        mediaPlayer.stopAsync();
#else
        mediaPlayer.stop();
#endif
        while (is_vlc_playing)//ensure vlc is done with surface. 
            Sleep(0);
        if (surface)
            delete surface;
        surface = nullptr;
        if (next)
            delete next;
        Debug_Info_Movie("LibVlc_Movie: destroy: %s", path.c_str());
    };

    bool Play();
    bool Play(LARGE_INTEGER play_end_time_in_ticks);

    void Pause(bool pause) {
        if (isPlaying) {
            paused = pause;
            //using this function primarily when window is resizing or loosing focus.
            //need to use stop here instead of pause as this was causing crashes.
            if (pause) {
                position = mediaPlayer.position();
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
                is_stop_set = true;
                mediaPlayer.stopAsync();
                while (is_vlc_playing)//ensure vlc is done with surface. 
                    Sleep(0);
#else
                mediaPlayer.stop();
#endif
            }
            else {
                mediaPlayer.play();
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
                mediaPlayer.setPosition(position, false);
#else
                mediaPlayer.setPosition(position);
#endif
                //subtitles must be re-initialised whenever media playback is stopped. subtitles can not be setup until media is actually playing - on_play() function called.
                while (!is_vlc_playing && !isError)
                    Sleep(0);
                Initialise_Subtitles();
                Initialise_Audio();
            }
            //mediaPlayer.setPause(pause);
        }
        if (next)
            next->Pause(pause);
        //isPlaying = false;
    };
    void SetMedia(std::string path);

    void Stop() {

#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
        is_stop_set = true;
        mediaPlayer.stopAsync();
        while (is_vlc_playing)//ensure vlc is done with surface. 
            Sleep(0);
#else
        mediaPlayer.stop();
#endif
        isPlaying = false;

        Debug_Info_Movie("LibVlc_Movie: Stop: %s", path.c_str());
        if (next)
            next->Stop();
    };
    bool IsPlaying() const {
        return isPlaying;
    }
    bool IsPlaying(LONG* p_current_branch, LONG* p_current_branch_list_num) {

        if (hasPlayed && !isPlaying && next)
            return next->IsPlaying(p_current_branch, p_current_branch_list_num);

        if (p_current_branch)
            *p_current_branch = branch;
        if (p_current_branch_list_num)
            *p_current_branch_list_num = list_num;

        return isPlaying;
    }
    bool IsPlaying(MOVIE_STATE* p_movie_state) {

        if (hasPlayed && !isPlaying && next)
            return next->IsPlaying(p_movie_state);

        if (p_movie_state) {
            p_movie_state->branch = branch;
            p_movie_state->list_num = list_num;
            p_movie_state->isPlaying = isPlaying;
            p_movie_state->hasPlayed = hasPlayed;
            p_movie_state->isError = isError;
        }

        return isPlaying;
    }
    bool HasPlayed() const {
        return hasPlayed;
    }
    bool HasPlayed(LONG* p_current_branch, LONG* p_current_branch_list_num) {
        if (hasPlayed) {
            if (p_current_branch)
                *p_current_branch = branch;
            if (p_current_branch_list_num)
                *p_current_branch_list_num = list_num;
            if (next)
                return next->HasPlayed(p_current_branch, p_current_branch_list_num);
        }
        return hasPlayed;
    }
    bool IsError() {
        if (!isError && next)
            return next->IsError();
        return isError;
    };
    void SetScale() {
        if (surface) {
            surface->ScaleTo((float)clientWidth, (float)clientHeight, SCALE_TYPE::fit);
            Debug_Info_Movie("LibVlc_Movie: SetScale: %s", path.c_str());
        }
        if (next)
            next->SetScale();
    }
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
    RenderTarget* Set_Surface(unsigned int width, unsigned int height) {
        if (!surface || width != surface->GetWidth() || height < surface->GetHeight()) {
            if (surface)
                delete surface;
            surface = new RenderTarget(0, 0, width, height, 32, 0);
        }
        return surface;
    }
    RenderTarget* Get_Surface() {
        return surface;
    }
    bool IsReadyToDisplay() const {
        if (!initialised_for_play || !isPlaying)
            return false;
        return true;
    }
    void SetReadyForDisplay() {
        if (!initialised_for_play) {
            Debug_Info_Movie("LibVlc_Movie: initialised_for_play SetReadyForDisplay %s", path.c_str());
            InitialiseForPlay_End();
        }
    }
    void Destroy_Surface() {
        cleanup();
    };
    RenderTarget* Get_Currently_Playing_Surface() {
        if (hasPlayed && !isPlaying && next)
            return next->Get_Currently_Playing_Surface();
        return surface;
    }
#else
    DrawSurface* Get_Currently_Playing_Surface() {
        if (hasPlayed && !isPlaying && next)
            return next->Get_Currently_Playing_Surface();
        return surface;
    }
#endif
protected:
private:
    LibVlc_Movie* next;
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
    VLC::MediaPlayer mediaPlayer = VLC::MediaPlayer(vlc_instance);
#else
    VLC::MediaPlayer mediaPlayer = VLC::MediaPlayer(vlc_instance);
#endif
    std::string path;
    LONG branch;
    LONG list_num;
    bool isPlaying;
    bool hasPlayed;
    bool isError;
    bool paused;
    bool initialised_for_play;
    float position;
    bool is_vlc_playing;
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
    bool is_stop_set;//when stopping after being deliberatly stopped rather than reaching the end of the movie. 
    RenderTarget* surface;
#else
    DrawSurface* surface;
#endif

    bool InitialiseForPlay_Start();
    void InitialiseForPlay_End();
    void Initialise_Subtitles();
    void Initialise_Audio();

    void on_play() {
        is_vlc_playing = true;
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
        is_stop_set = false;
#endif
        Debug_Info_Movie("LibVlc_Movie: on_play Play started: %s", path.c_str());

    };
    void on_stopped() {
        is_vlc_playing = false;
        if (!paused) {
            isPlaying = false;
            Debug_Info_Movie("LibVlc_Movie: on_stopped Play stopped: %s", path.c_str());
        }
    };
    void on_encountered_error() {
        isError = true;
        Debug_Info_Error("LibVlc_Movie: Error Encountered !!!: %s", path.c_str());
    };
    void on_buffering(float percent) {
       // if (percent == 0.0f)
       //     Initialise_Subtitles();
        
        //if (percent == 100.0f && !initialised_for_play) {
       //     Debug_Info("LibVlc_Movie: Buffering %f: %s", percent, path.c_str());
        //    InitialiseForPlay_End();
        //}
    };
    void on_end_reached() {
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
        //libvlc v4 has replace onEndReached with onStopping.
        //have to check if it was stopped(!isPlaying) or ending.
        if (is_stop_set || isError)
            return;
#endif
        Debug_Info_Movie("LibVlc_Movie: end reached: %s", path.c_str());
        if (next)
            next->Play();
        isPlaying = false;
        hasPlayed = true;

    };
    void on_time_changed(libvlc_time_t time_ms);
#if LIBVLC_VERSION_INT < LIBVLC_VERSION(4, 0, 0, 0)
    void* lock(void** planes) {
        if (!initialised_for_play) {
            Debug_Info_Movie("LibVlc_Movie: initialised_for_play ON FIRST LOCK %s", path.c_str());
            InitialiseForPlay_End();
        }
        //Debug_Info("lock: %s", path.c_str());
        BYTE* pSurface = nullptr;
        LONG pitch = 0;

        if (surface->Lock((VOID**)&pSurface, &pitch) != S_OK) {
            Debug_Info_Error("LibVlc_Movie: LOCK FAILED %s", path.c_str());
            return nullptr;
        }
        *planes = (VOID**)pSurface;
        return nullptr;

    };
    void unlock(void* picture, void* const* planes) {
        //Debug_Info("unlock: %s", path.c_str());
        surface->Unlock();
        if (!initialised_for_play || !isPlaying)
            return;
    };
    void display(void* picture) const {
        if (!initialised_for_play || !isPlaying)
            return;
        Display_Dx_Present(PRESENT_TYPE::movie);
    };
    uint32_t format(char* chroma, uint32_t* width, uint32_t* height, uint32_t* p_pitch, uint32_t* lines) {
        memcpy(chroma, "BGRA", 4);
        Debug_Info_Movie("LibVlc_Movie: setVideoFormatCallbacks w:%d, h:%d", *width, *height);
        if (!surface || *width != surface->GetWidth() || *height < surface->GetHeight() || surface->GetPixelWidth() != 4) {
            if (surface)
                delete surface;
            surface = new DrawSurface(0, 0, *width, *height, 32, 0);
            surface->ScaleTo((float)clientWidth, (float)clientHeight, SCALE_TYPE::fit);
            Debug_Info_Movie("LibVlc_Movie: surface created: %s", path.c_str());
        }
        BYTE* pSurface = nullptr;
        LONG pitch = 0;
        if (surface->Lock((VOID**)&pSurface, &pitch) != S_OK)
            return 0;
        surface->Unlock();
        *width = surface->GetWidth();
        *height = surface->GetHeight();
        *p_pitch = pitch;
        *lines = *height;
        return 1;
    };
#endif
    void cleanup() {
        Debug_Info_Movie("LibVlc_Movie: cleanup: %s", path.c_str());
        //if (surface)
        //    delete surface;
        //surface = nullptr;
    };

};



class LibVlc_MovieInflight {
public:
    LibVlc_MovieInflight(const char* file_name, RECT* p_rc_gui_unscaled, DWORD start_frame_30, DWORD length_frames_30);
    // For In-flight IFF files modified to use an alphabetical letter value to differentiate each scene. These letters relate to those appended to the name of each individual scene movie file.
    // The file name specified in these IFF's "PROF\RADI\FMV " section, should have no file extension. Each scene is numbered correlating with letters in the alphabet starting with 0=a.
    // These values are stored in place of the time-code duration value found in unmodified IFF's "PROF\RADI\MSGS" section.
    LibVlc_MovieInflight(const char* file_name, RECT* p_rc_gui_unscaled, DWORD appendix);
    ~LibVlc_MovieInflight() {
        Stop();
        while (is_vlc_playing)//ensure vlc is done with surface. 
            Sleep(0);
        if (surface)
            delete surface;
        surface = nullptr;
        if (surface_bg)
            delete surface_bg;
        surface_bg = nullptr;
        Debug_Info_Movie("LibVlc_MovieInflight: destroy: %s", path.c_str());
    };
    bool Check_Play_Time() {
        //Debug_Info("LibVlc_MovieInflight: Check_Play_Time: %d - %d", (int)mediaPlayer.time(), (int)time_ms_length);
        if (play_setup_complete && time_ms_length) {
            LARGE_INTEGER current_time_in_ticks;
            QueryPerformanceCounter(&current_time_in_ticks);
            if (current_time_in_ticks.QuadPart >= play_end_time_in_ticks.QuadPart) {
                Stop();
                hasPlayed = true;
            }
        }
        return play_setup_complete;
    };
    bool Play();
    void Pause(bool pause) {
        if (isPlaying) {
            paused = pause;
            //using this function primarily when window is resizing or loosing focus.
            //need to use stop here instead of pause as this was causing crashes.
            if (pause) {
                position = mediaPlayer.position();
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
                mediaPlayer.stopAsync();
                while (is_vlc_playing)//ensure vlc is done with surface. 
                    Sleep(0);
#else
                mediaPlayer.stop();
#endif
            }
            else {
                mediaPlayer.play();
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
                mediaPlayer.setPosition(position, false);
#else
                mediaPlayer.setPosition(position);
#endif
                //subtitles must be re-initialised whenever media playback is stopped. subtitles can not be setup until media is actually playing - on_play() function called.
                while (!is_vlc_playing && !isError)
                    Sleep(0);
                //disable subtitles, we don't want subs overlayed on the inflight video.
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
                libvlc_media_player_unselect_track_type(mediaPlayer, libvlc_track_type_t::libvlc_track_text);
#else
                mediaPlayer.setSpu(-1);
#endif
            }
        }
    };
    void SetMedia(std::string path);

    void Stop() {

#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
        mediaPlayer.stopAsync();
#else
        mediaPlayer.stop();
#endif
        isPlaying = false;

        Debug_Info_Movie("LibVlc_MovieInflight: Stop: %s", path.c_str());
    };

    bool IsPlaying() const {
        return isPlaying;
    }
    //bool IsPlayInitialised() const {
    //    return play_setup_complete;// play_counter_started;
    //}
    bool HasPlayed() const {
        return hasPlayed;
    }
    bool HasAudio() const {
        return has_audio;
    }
    bool IsError() const {
        return isError;
    };
    void Display();
    void Update_Display_Dimensions(RECT* p_rc_gui_unscaled);

protected:
private:
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
    VLC::MediaPlayer mediaPlayer = VLC::MediaPlayer(vlc_instance);
#else
    VLC::MediaPlayer mediaPlayer = VLC::MediaPlayer(vlc_instance);
#endif
    std::string path;
    bool isPlaying;
    bool hasPlayed;
    bool isError;
    bool paused;
    bool play_setup_start;
    bool play_setup_complete;
    bool has_audio;
    float position;
    bool is_vlc_playing;
    DrawSurface* surface;
    DrawSurface8_RT* surface_bg;
    RECT rc_dest_unscaled;

    libvlc_time_t time_ms_length;
    libvlc_time_t time_ms_start;
    LARGE_INTEGER play_end_time_in_ticks;

    void Initialise_Audio();
    void libvlc_movieinflight_initialise();

    void initialise_for_play();

    void on_play() {
        is_vlc_playing = true;
        //set flag to initialise video setup in "initialise_for_play()".
        play_setup_start = true;

        Debug_Info_Movie("LibVlc_MovieInflight: On Play: %s", path.c_str());

    };
    void on_stopped() {
        is_vlc_playing = false;
        if (!paused) {
            isPlaying = false;
            Debug_Info_Movie("LibVlc_MovieInflight: OnStop stopped: %s", path.c_str());
        }
    };
    void on_encountered_error() {
        isError = true;
        Debug_Info_Error("LibVlc_MovieInflight: Error Encountered !!!: %s", path.c_str());
    };
    void on_buffering(float percent) {
         //if (percent == 100.0f && !initialised_for_play) {
         //    Debug_Info("LibVlc_MovieInflight: Buffering %f: %s", percent, path.c_str());
         //}
    };
    void on_end_reached() {
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
        //libvlc v4 has replace onEndReached with onStopping.
        //have to check if it was stopped(!isPlaying) or ending.
        if (!isPlaying || isError)
            return;
#endif
        Debug_Info_Movie("LibVlc_MovieInflight: end reached: %s", path.c_str());
        isPlaying = false;
        hasPlayed = true;
    };

    void on_time_changed(libvlc_time_t time_ms) {
    }
    void* lock(void** planes) {
        //set flag to initialise playback timer in "initialise_for_play()".
        //play_counter_start = true;
        initialise_for_play();
        //Debug_Info("lock: %s", path.c_str());
        BYTE* pSurface = nullptr;
        LONG pitch = 0;

        if (surface->Lock((VOID**)&pSurface, &pitch) != S_OK) {
            Debug_Info_Error("LibVlc_MovieInflight: LOCK FAILED %s", path.c_str());
            return nullptr;
        }
        *planes = (VOID**)pSurface;
        return nullptr;

    };
    void unlock(void* picture, void* const* planes) {
        //Debug_Info("unlock: %s", path.c_str());
        surface->Unlock();
    };
    void display(void* picture) const {
        // not used, causes graphic artifacts
        //if (!initialisation_complete)
        //     return;
        // Display_Dx_Present(PRESENT_TYPE::space);
    };
    uint32_t format(char* chroma, uint32_t* width, uint32_t* height, uint32_t* p_pitch, uint32_t* lines) {
        memcpy(chroma, "BGRA", 4);
        Debug_Info_Movie("LibVlc_MovieInflight: setVideoFormatCallbacks w:%d, h:%d", *width, *height);
        if (!surface || *width != surface->GetWidth() || *height < surface->GetHeight() || surface->GetPixelWidth() != 4) {
            if (surface)
                delete surface;
            surface = new DrawSurface(0, 0, *width, *height, 32, 0);
            Update_Display_Dimensions(nullptr);
            Debug_Info_Movie("LibVlc_MovieInflight: surface created: %s", path.c_str());
        }
        BYTE* pSurface = nullptr;
        LONG pitch = 0;
        if (surface->Lock((VOID**)&pSurface, &pitch) != S_OK)
            return 0;
        surface->Unlock();
        *width = surface->GetWidth();
        *height = surface->GetHeight();
        *p_pitch = pitch;
        *lines = *height;
        return 1;
    };
    void cleanup() {
        Debug_Info_Movie("LibVlc_MovieInflight: cleanup: %s", path.c_str());
        if (surface)
            delete surface;
        surface = nullptr;
    };

};

extern LibVlc_Movie* pMovie_vlc;
extern LibVlc_MovieInflight* pMovie_vlc_Inflight;

BOOL Get_Movie_Name_From_Path(const char* mve_path, std::string* p_retPath);
