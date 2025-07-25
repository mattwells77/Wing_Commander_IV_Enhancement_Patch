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

#include "pch.h"

#include "movies_vlclib.h"
#include "configTools.h"
#include "wc4w.h"

//#include <iostream>
//#include <thread>
//#include <cstring>
//using namespace VLC;

/*const char* const vlc_options[] = {
    "--file-caching=300"//,
    //"--network-caching=150",
    //"--clock-jitter=0",
    //"--live-caching=150",
    //"--clock-synchro=0",
    //"-vvv",
    //"--drop-late-frames",
    //"--skip-frames"
     };*/
//const char* const vlc_options[] = { "--freetype-font=Incised901 Lt BT" };

//VLC::Instance vlc_instance = VLC::Instance(_countof(vlc_options), vlc_options);
VLC::Instance vlc_instance = VLC::Instance(0, nullptr);

std::string movie_dir;
std::string movie_ext;
std::string movie_config_path;

BOOL are_movie_path_setting_set = FALSE;
int branch_offset_ms = 0;

BOOL is_inflight_mono_shader_enabled = FALSE;
BOOL inflight_use_audio_from_file_if_present = 1;
SCALE_TYPE inflight_display_aspect_type = SCALE_TYPE::fit;

DWORD inflight_cockpit_bg_colour_argb = 0xFF000000;

LibVlc_MovieInflight* pMovie_vlc_Inflight = nullptr;

//____________________________________________________________________________________
static BOOL Create_Movie_Path_Inflight(const char* movie_name, std::string* p_retPath) {
    if (!p_retPath)
        return FALSE;

    p_retPath->clear();
    p_retPath->append(movie_dir);
    p_retPath->append("inflight\\");
    p_retPath->append(movie_name);
    p_retPath->append(movie_ext);


    Debug_Info_Movie("Create_Movie_Path_Inflight: %s", p_retPath->c_str());
    if (GetFileAttributesA(p_retPath->c_str()) == INVALID_FILE_ATTRIBUTES)
        return FALSE;

    return TRUE;
}


//___________________________________________________________________________________________________________
static BOOL Create_Movie_Path_Inflight(const char* movie_name, DWORD appendix_offset, std::string* p_retPath) {
    if (!p_retPath)
        return FALSE;
    if (appendix_offset == -1)
        return Create_Movie_Path_Inflight(movie_name, p_retPath);
    if (appendix_offset >= 26) {
        Debug_Info_Error("Create_Movie_Path_Inflight: appendix greater than 26: appendix:%d", appendix_offset);
        return FALSE;
    }
    char appendix[2] = { 'a'+ (char)appendix_offset , '\0'};

    p_retPath->clear();
    p_retPath->append(movie_dir);
    p_retPath->append("inflight\\");
    p_retPath->append(movie_name);
    p_retPath->append("_");
    p_retPath->append(appendix);
    p_retPath->append(movie_ext);


    Debug_Info_Movie("Create_Movie_Path_Inflight: %s", p_retPath->c_str());
    if (GetFileAttributesA(p_retPath->c_str()) == INVALID_FILE_ATTRIBUTES)
        return FALSE;

    return TRUE;
}



//__________________________________________
static bool isPathRelative(const char* path) {

    if (isalpha(path[0])) {
        if (path[1] == ':' && path[2] == '\\')
            return false;
    }
    else if (path[0] == '\\')
        return false;
    return true;
}


//Set the absolute path for movie config file.
//_________________________________
static void Set_Movie_Config_Path() {
    movie_config_path.clear();
    if (isPathRelative(movie_dir.c_str())) {
        char* pMoviePath = new char[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, pMoviePath);
        movie_config_path.append(pMoviePath);
        movie_config_path.append("\\");
        delete[] pMoviePath;
    }
    movie_config_path.append(movie_dir);
    movie_config_path.append("movies.ini");
}


//______________________________
static BOOL Set_Movie_Settings() {
    if (are_movie_path_setting_set)
        return TRUE;
    
    char* char_buff = new char[MAX_PATH];
    char* char_ptr = char_buff;
  
    wchar_t* wchar_buff = new wchar_t[MAX_PATH];
    size_t num_bytes = 0;

    //set the hd movie directory.
    ConfigReadString(L"MOVIES", L"PATH", CONFIG_MOVIES_PATH, wchar_buff, MAX_PATH);
    //convert wstring to string
    wcstombs_s(&num_bytes, char_buff, MAX_PATH, wchar_buff, _TRUNCATE);

    //skip over any space chars
    while (*char_ptr == ' ')
        char_ptr++;
    
    movie_dir = char_ptr;
    //make sure path ends with a back-slash.
    if (movie_dir.at(movie_dir.length() - 1) != '\\')
        movie_dir.append("\\");
    Debug_Info_Movie("movie directory: %s", movie_dir.c_str());

    Set_Movie_Config_Path();
    Debug_Info_Movie("movie_config_path: %s", movie_config_path.c_str());

    //get the hd movie extension.
    ConfigReadString(L"MOVIES", L"EXT", CONFIG_MOVIES_EXT, wchar_buff, MAX_PATH);
    //convert wstring to string
    wcstombs_s(&num_bytes, char_buff, MAX_PATH, wchar_buff, _TRUNCATE);

    //skip over any space chars
    char_ptr = char_buff;
    while (*char_ptr == ' ')
        char_ptr++;
    
    movie_ext.clear();
    ////make sure extension begins with a dot.
    if (char_ptr[0] != '.')
        movie_ext.append(".");
    movie_ext.append(char_ptr);
    Debug_Info_Movie("movie extension: %s", movie_ext.c_str());

    //get the offset between movie branches in milliseconds.
    branch_offset_ms = ConfigReadInt(L"MOVIES", L"BRANCH_OFFSET_MS", CONFIG_MOVIES_BRANCH_OFFSET_MS);
    Debug_Info_Movie("movie branch offset: %d ms", branch_offset_ms);


    inflight_use_audio_from_file_if_present = ConfigReadInt(L"MOVIES", L"INFLIGHT_USE_AUDIO_FROM_FILE_IF_PRESENT", CONFIG_MOVIES_INFLIGHT_USE_AUDIO_FROM_FILE_IF_PRESENT);
    if(ConfigReadInt(L"MOVIES", L"INFLIGHT_DISPLAY_ASPECT_TYPE", CONFIG_MOVIES_INFLIGHT_DISPLAY_ASPECT_TYPE) == 0)
        inflight_display_aspect_type = SCALE_TYPE::fill;
    else
        inflight_display_aspect_type = SCALE_TYPE::fit;

    inflight_cockpit_bg_colour_argb = 0xFF000000 | ConfigReadInt(L"MOVIES", L"INFLIGHT_COCKPIT_BG_COLOUR_RGB", CONFIG_MOVIES_INFLIGHT_COCKPIT_BG_COLOUR_RGB);

    if (ConfigReadInt(L"MOVIES", L"INFLIGHT_MONO_SHADER_ENABLE", CONFIG_INFLIGHT_MONO_SHADER_ENABLE)) {
        is_inflight_mono_shader_enabled = TRUE;
        DWORD colour = ConfigReadInt(L"MOVIES", L"INFLIGHT_MONO_SHADER_COLOUR", CONFIG_INFLIGHT_MONO_SHADER_COLOUR);
        UINT brightness = ConfigReadInt(L"MOVIES", L"INFLIGHT_MONO_SHADER_BRIGHTNESS", CONFIG_INFLIGHT_MONO_SHADER_BRIGHTNESS);
        UINT contrast = ConfigReadInt(L"MOVIES", L"INFLIGHT_MONO_SHADER_CONTRAST", CONFIG_INFLIGHT_MONO_SHADER_CONTRAST);
        Inflight_Mono_Colour_Setup(colour, brightness, contrast);
    }

    delete[] char_buff;
    delete[] wchar_buff;
    return are_movie_path_setting_set = TRUE;
}


//_________________________________________________________________________
BOOL Get_Movie_Name_From_Path(const char* mve_path, std::string* p_retPath) {
    
    if (!p_retPath)
        return FALSE;
    p_retPath->clear();

    char movie_name[9]{ 0 };

    const char* movie_name_start = strrchr(mve_path, '\\');

    if (movie_name_start) {
        movie_name_start += 1;
        strncpy_s(movie_name, _countof(movie_name), movie_name_start, _countof(movie_name) - 1);
        char* ext = strrchr(movie_name, '.');
        if (ext) {
            *ext = '\0';
        }
        char* c = movie_name;
        while (*c) {
            *c = tolower(*c);
            c++;
        }

        /*Debug_Info_Movie("movie name original: %s", movie_name);
        GetPrivateProfileStringA(movie_name, "name", movie_name, movie_name, _countof(movie_name), movie_config_path.c_str());
        //end string if a comment or space char is encountered.
        char* end_char = strchr(movie_name, ';');
        if (end_char)
            *end_char = '\0';
        end_char = strchr(movie_name, ' ');
        if (end_char)
            *end_char = '\0';

        Debug_Info_Movie("movie name fixed: %s", movie_name);*/
        p_retPath->assign(movie_name);
    }
    Debug_Info_Movie("Get_Movie_Name_From_Path: %s", p_retPath->c_str());

    return TRUE;
}


//returns 1 branch path valid, 0 branch path invalid, -1 skip this branch.
//___________________________________________________________________________________________________
static BOOL Create_Movie_Path_Name_Branch(const char* movie_name, int branch, std::string* p_retPath) {

    char c_branch[3] = "\0";
    if (branch < 26) {
        c_branch[0] = 'a' + branch;
        c_branch[1] = '\0';
    }
    else {
        branch -= 26;
        c_branch[0] = 'a' + branch;
        c_branch[1] = 'a' + branch;
        c_branch[2] = '\0';
    }

    char movie_name_fixed[9]{ 0 };
    Debug_Info_Movie("movie name original: %s", movie_name);
    GetPrivateProfileStringA(movie_name, "name", movie_name, movie_name_fixed, _countof(movie_name_fixed), movie_config_path.c_str());
    //end string if a comment or space char is encountered.
    char* end_char = strchr(movie_name_fixed, ';');
    if (end_char)
        *end_char = '\0';
    end_char = strchr(movie_name_fixed, ' ');
    if (end_char)
        *end_char = '\0';
    Debug_Info_Movie("movie name fixed: %s", movie_name_fixed);

    Debug_Info_Movie("c_branch original: %s", c_branch);
    GetPrivateProfileStringA(movie_name, c_branch, c_branch, c_branch, _countof(c_branch), movie_config_path.c_str());
    //end string if a comment or space char is encountered.
    end_char = strchr(c_branch, ';');
    if (end_char)
        *end_char = '\0';
    end_char = strchr(c_branch, ' ');
    if (end_char)
        *end_char = '\0';

    Debug_Info_Movie("c_branch fixed: %s", c_branch);

    //if c_branch == -1, marks this branch to be skipped
    if (strncmp(c_branch, "-1", 2) == 0)
        return -1;

    if (!p_retPath)
        return FALSE;

    p_retPath->assign(movie_dir);
    p_retPath->append(movie_name_fixed);
    p_retPath->append(c_branch);
    p_retPath->append(movie_ext);

    //Debug_Info("Create_Movie_Path: %s", p_retPath->c_str());
    if (GetFileAttributesA(p_retPath->c_str()) == INVALID_FILE_ATTRIBUTES)
        return FALSE;

    return TRUE;
}


#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
//_________________________________________________________________________________________________________________________________
static bool video_output_setup_cb(void** opaque, const libvlc_video_setup_device_cfg_t* cfg, libvlc_video_setup_device_info_t* out) {
    Debug_Info_Movie("LibVlc_Movie: video_output_setup_cb");
    //LibVlc_Movie* libvlc_movie = static_cast<LibVlc_Movie*>(*opaque);
    out->d3d11.device_context = GetD3dDeviceContext();
    out->d3d11.context_mutex = nullptr;
    return true;
}

//_______________________________________________
static void video_output_cleanup_cb(void* opaque) {
    Debug_Info_Movie("LibVlc_Movie: video_output_cleanup_cb");
    LibVlc_Movie* libvlc_movie = static_cast<LibVlc_Movie*>(opaque);
    libvlc_movie->Destroy_Surface();

    Reset_DX11_Shader_Defaults();
}

//____________________________________________________________________________________________________________________
static bool video_update_output_cb(void* opaque, const libvlc_video_render_cfg_t* cfg, libvlc_video_output_cfg_t* out) {

    LibVlc_Movie* libvlc_movie = static_cast<LibVlc_Movie*>(opaque);
    unsigned width = cfg->width;
    if (width == 0)
        width = 8;
    unsigned height = cfg->height;
    if (height == 0)
        height = 8;
    Debug_Info_Movie("LibVlc_Movie: video_update_output_cb %d, %d", width, height);

    libvlc_movie->Set_Surface(width, height);
    RenderTarget* surface = libvlc_movie->Get_Surface();
    surface->ScaleTo((float)clientWidth, (float)clientHeight, SCALE_TYPE::fit);

    out->dxgi_format = DXGI_FORMAT_B8G8R8A8_UNORM;
    out->full_range = true;
    out->colorspace = libvlc_video_colorspace_BT709;
    out->primaries = libvlc_video_primaries_BT709;
    out->transfer = libvlc_video_transfer_func_SRGB;
    out->orientation = libvlc_video_orient_top_left;

    return true;
}

//_____________________________________
static void video_swap_cb(void* opaque) {
    //Debug_Info_Movie("LibVlc_Movie: video_swap_cb");
    LibVlc_Movie* libvlc_movie = static_cast<LibVlc_Movie*>(opaque);
    if (!libvlc_movie->IsReadyToDisplay())
        return;
    Display_Dx_Present(PRESENT_TYPE::movie);
}

//________________________________________________________
static bool video_makeCurrent_cb(void* opaque, bool enter) {

    LibVlc_Movie* libvlc_movie = static_cast<LibVlc_Movie*>(opaque);
    libvlc_movie->SetReadyForDisplay();
    if (!libvlc_movie->IsReadyToDisplay())
        return false;
    RenderTarget* surface = libvlc_movie->Get_Surface();
    if (!surface)
        return false;
    if (enter) {
        //Debug_Info_Movie("LibVlc_Movie: video_makeCurrent_cb enter");
        surface->ClearRenderTarget(GetDepthStencilView());
    }
    else {
        //Debug_Info_Movie("LibVlc_Movie: video_makeCurrent_cb exit");
        Reset_DX11_Shader_Defaults();
    }
    return true;
}

//_____________________________________________________________________________
static bool video_output_select_plane_cb(void* opaque, size_t plane, void* out) {
    //Debug_Info_Movie("LibVlc_Movie: video_output_select_plane_cb");
    ID3D11RenderTargetView** output = static_cast<ID3D11RenderTargetView**>(out);
    LibVlc_Movie* libvlc_movie = static_cast<LibVlc_Movie*>(opaque);
    RenderTarget* surface = libvlc_movie->Get_Surface();
    if (!surface)
        return false;
    if (plane != 0) //DXGI_FORMAT_R8G8B8A8_UNORM should only have the one plane.
        return false;
    //return ID3D11RenderTargetView.
    *output = surface->GetRenderTargetView();
    return true;
}
#endif


//_________________________________________________________________________________________
LibVlc_Movie::LibVlc_Movie(std::string movie_name, LONG* branch_list, LONG branch_list_num) {
    //Debug_Info("LibVlc_Movie: Create Start");
    Set_Movie_Settings();
    next = nullptr;
    isPlaying = false;
    hasPlayed = false;
    isError = false;

    surface = nullptr;
    initialised_for_play = false;

    position = 0;
    paused = false;
    is_vlc_playing = false;
    
    using namespace std::placeholders; // for `_1`

    mediaPlayer.eventManager().onPlaying(std::bind(&LibVlc_Movie::on_play, this));
    mediaPlayer.eventManager().onStopped(std::bind(&LibVlc_Movie::on_stopped, this));

    mediaPlayer.eventManager().onEncounteredError(std::bind(&LibVlc_Movie::on_encountered_error, this));
    mediaPlayer.eventManager().onTimeChanged(std::bind(&LibVlc_Movie::on_time_changed, this, _1));

    mediaPlayer.eventManager().onBuffering(std::bind(&LibVlc_Movie::on_buffering, this, _1));
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
    is_stop_set = false;
    mediaPlayer.eventManager().onStopping(std::bind(&LibVlc_Movie::on_end_reached, this));
#else
    mediaPlayer.eventManager().onEndReached(std::bind(&LibVlc_Movie::on_end_reached, this));
#endif
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
    libvlc_video_set_output_callbacks(mediaPlayer, libvlc_video_engine_d3d11,
        video_output_setup_cb, video_output_cleanup_cb, nullptr,
        video_update_output_cb, video_swap_cb, video_makeCurrent_cb,
        nullptr, nullptr, video_output_select_plane_cb, this);
#else
    mediaPlayer.setVideoCallbacks(std::bind(&LibVlc_Movie::lock, this, _1), std::bind(&LibVlc_Movie::unlock, this, _1, _2), std::bind(&LibVlc_Movie::display, this, _1));
    mediaPlayer.setVideoFormatCallbacks(std::bind(&LibVlc_Movie::format, this, _1, _2, _3, _4, _5), std::bind(&LibVlc_Movie::cleanup, this));
#endif

    list_num = branch_list_num;
    if (branch_list)
        branch = branch_list[list_num];
    else
        branch = -1;

    if (branch >= 0) {
        BOOL branch_state = -1;
        while (branch_state == -1 && branch >= 0) {//find the first branch not marked to be skipped.
            branch_state = Create_Movie_Path_Name_Branch(movie_name.c_str(), branch, &path);
            if (branch_state == -1) {//branch to be skipped
                Debug_Info_Movie("LibVlc_Movie: skipping branch:%d", branch);
                list_num++;
                branch = branch_list[list_num];
            }
        }
        if (branch_state == 0 || branch < 0) {
            Debug_Info_Error("LibVlc_Movie: Create Movie Branch Path FAILED: %s, branch:%d", movie_name.c_str(), branch);
            isError = true;
        }
        else {
            Debug_Info_Movie("LibVlc_Movie: Created Movie Branch Path: %s", path.c_str());

            LONG next_list_num = list_num + 1;
            LONG next_branch = branch_list[next_list_num];
            if (next_branch >= 0) {
                branch_state = -1;
                while (branch_state == -1 && next_branch >= 0) {//find the next branch not marked to be skipped.
                    branch_state = Create_Movie_Path_Name_Branch(movie_name.c_str(), next_branch, nullptr);
                    if (branch_state == -1) {//next branch to be skipped
                        Debug_Info_Movie("LibVlc_Movie: skipping next branch:%d", next_branch);
                        next_list_num++;
                        next_branch = branch_list[next_list_num];
                    }
                }
                if (branch_state != -1 && next_branch >= 0)
                    next = new LibVlc_Movie(movie_name, branch_list, next_list_num);
            }
        }
    }
    else
        Debug_Info_Error("LibVlc_Movie: FAILED Invalid Branch:%d, %s", branch, movie_name.c_str());

    //Debug_Info("LibVlc_Movie: Create Done: %s", path.c_str());
}


//_______________________
bool LibVlc_Movie::Play() {
    if (isError)
        isPlaying = false;
    else if (initialised_for_play) {
        Debug_Info_Movie("LibVlc_Movie: Play: %s", path.c_str());
        mediaPlayer.setPause(false);
        if (next)
            next->InitialiseForPlay_Start();

        isPlaying = true;
        if (hasPlayed)
            isPlaying = false;
        
        //Display branch subtitle if present. eg. text displayed when jumping to a different solar system. 
        if (isPlaying && p_wc4_movie_branch_subtitle[list_num]) {
            char* text = p_wc4_movie_branch_subtitle[list_num] + 1;
            BYTE text_length = p_wc4_movie_branch_subtitle[list_num][0];
            Debug_Info_Movie("LibVlc_Movie: Play branch sub: length:%d, %s", text_length, text);
            mediaPlayer.setMarqueeString(libvlc_marquee_Text, text);
            //mediaPlayer.setMarqueeInt(libvlc_marquee_Size, 18);
            mediaPlayer.setMarqueeInt(libvlc_marquee_Position, 8);
            mediaPlayer.setMarqueeInt(libvlc_marquee_Timeout, 8000);
            mediaPlayer.setMarqueeInt(libvlc_marquee_Enable, 1);
        }

        return isPlaying;
    }
    else {
        if (InitialiseForPlay_Start())
            return Play();

    }

    return isPlaying;
}


//___________________________________________________________
bool LibVlc_Movie::Play(LARGE_INTEGER play_end_time_in_ticks) {
    if (isError)
        isPlaying = false;
    else if (initialised_for_play) {
        Debug_Info_Movie("LibVlc_Movie: Play: %s", path.c_str());
        LARGE_INTEGER current_time_in_ticks;
        QueryPerformanceCounter(&current_time_in_ticks);
        if (next)
            next->InitialiseForPlay_Start();
        //offset play_end_time_in_ticks for for next movie
        //convert ms to ticks, -180ms seems about right.
        play_end_time_in_ticks.QuadPart += branch_offset_ms * p_wc4_frequency->QuadPart / 1000LL;
        //wait untill the right moment to start the next movie. 
        while (current_time_in_ticks.QuadPart < play_end_time_in_ticks.QuadPart) {
            Sleep(0);
            QueryPerformanceCounter(&current_time_in_ticks);
        }
        mediaPlayer.setPause(false);

        isPlaying = true;
        if (hasPlayed)
            isPlaying = false;
        return isPlaying;
    }
    else {
        if (InitialiseForPlay_Start())
            return Play();

    }

    return isPlaying;
}


//___________________________________________
void LibVlc_Movie::SetMedia(std::string path) {
    VLC::Media media;

#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
    media = VLC::Media(path, VLC::Media::FromPath);
    //////////////libvlc subtitle autodetection path (bug) work-around////////////////////////////////////////////
    //To-Do - Check for this issue in later versions. Current version = vlc-4.0.0-dev-win32-3db080cb
    //The current libvlc 4 nightly doesn't seem to scan the ".\subtitles" folders as the stable v3 does.
    //Needed to add the full path here, to get things working.
    std::string subs_path = "sub-autodetect-path=";
    subs_path.append(movie_dir);
    subs_path.append("subtitles\\");
    libvlc_media_add_option(media, subs_path.c_str());
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
#else
    Debug_Info_Movie("LibVlc_Movie: SetMedia: %s", path.c_str());
    media = VLC::Media(vlc_instance, path, VLC::Media::FromPath);

#endif
    mediaPlayer.setMedia(media);

    initialised_for_play = false;
}


//_______________________________________________________
void LibVlc_Movie::on_time_changed(libvlc_time_t time_ms) {
    if (next && !next->IsPlaying()) {
        libvlc_time_t mediaLength = mediaPlayer.length();
        if (mediaLength >= 0 && time_ms >= mediaLength - 1000) {
            Debug_Info_Movie("LibVlc_Movie: ON TIME CHANGED time_ms:%d:%d: dist:%d, %s", (LONG)(time_ms), (LONG)(mediaLength), (LONG)(mediaLength - time_ms), path.c_str());
            //Get the current number of ticks then add the number of tick till the end of the current movie.
            LARGE_INTEGER play_end_time_in_ticks;
            QueryPerformanceCounter(&play_end_time_in_ticks);
            play_end_time_in_ticks.QuadPart += (mediaLength - time_ms) * p_wc4_frequency->QuadPart / 1000LL;

            //Set the next movie up to play providing the time when the current movie should end.
            next->Play(play_end_time_in_ticks);

            LARGE_INTEGER play_init_end_time;
            QueryPerformanceCounter(&play_init_end_time);
            play_init_end_time.QuadPart -= play_end_time_in_ticks.QuadPart;
            play_init_end_time.QuadPart *= 1000LL;
            play_init_end_time.QuadPart /= p_wc4_frequency->QuadPart;
            Debug_Info_Movie("LibVlc_Movie: ON TIME CHANGED time taken: %d, %s", play_init_end_time.LowPart, path.c_str());
            isPlaying = false;
            hasPlayed = true;

        }
    }
}


//_______________________________________
void LibVlc_Movie::Initialise_Subtitles() {

#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
    VLC::MediaPtr media_p = mediaPlayer.media();
    //libvlc_media_parse_request(vlc_instance, *media_p, libvlc_media_parse_local, -1);
    libvlc_media_tracklist_t* p_sub_tracklist = libvlc_media_player_get_tracklist(mediaPlayer, libvlc_track_type_t::libvlc_track_text, false);
    size_t subCount = libvlc_media_tracklist_count(p_sub_tracklist);
#else
    size_t subCount = mediaPlayer.spuCount();
#endif
    Debug_Info_Movie("LibVlc_Movie: Subtitle track count: %d", subCount);

    if (*p_wc4_subtitles_enabled == 0)
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
        libvlc_media_player_unselect_track_type(mediaPlayer, libvlc_track_type_t::libvlc_track_text);
#else
        mediaPlayer.setSpu(-1);
#endif
    else {
        //Debug_Info_Movie("language ref: %d", *p_wc4_language_ref);
        std::string language;
        switch (*p_wc4_language_ref) {
        case 0:
            language = "[English]";
            break;
        case 1:
            language = "[German]";
            break;
        case 2:
            language = "[French]";
            break;
        default:
            return;
            break;
        }
        Debug_Info_Movie("LibVlc_Movie: Search for subtitles: %s", language.c_str());

#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
        for (size_t i = 0; i < subCount; i++) {
            libvlc_media_track_t* p_sub = libvlc_media_tracklist_at(p_sub_tracklist, i);
            std::string name = p_sub->psz_name;
            
            if (name.find(language) != std::string::npos) {
                Debug_Info_Movie("LibVlc_Movie: Subtitle track selected ID: %d, name: %s", p_sub->i_id, name.c_str());
                libvlc_media_player_select_track(mediaPlayer, p_sub);
            }
            
        }
        libvlc_media_tracklist_delete(p_sub_tracklist);
#else
        std::vector <VLC::TrackDescription> trackDescriptions = mediaPlayer.spuDescription();
        for (size_t i = 0; i < trackDescriptions.size(); i++) {
            VLC::TrackDescription description = trackDescriptions[i];
            //find a subtitle track description containing matching language text and set it for display.
            if (description.name().find(language) != std::string::npos) {
                Debug_Info_Movie("LibVlc_Movie: Subtitle track selected ID: %d, name: %s", description.id(), description.name().c_str());
                mediaPlayer.setSpu(description.id());
            }
        }
#endif
    }
}


//___________________________________
void LibVlc_Movie::Initialise_Audio() {
    return;
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
    VLC::MediaPtr media_p = mediaPlayer.media();
    //libvlc_media_parse_request(vlc_instance, *media_p, libvlc_media_parse_local, -1);
    libvlc_media_tracklist_t* p_audio_tracklist = libvlc_media_player_get_tracklist(mediaPlayer, libvlc_track_type_t::libvlc_track_audio, false);
    size_t audioCount = libvlc_media_tracklist_count(p_audio_tracklist);
#else
    size_t audioCount = mediaPlayer.audioTrackCount();
#endif
    Debug_Info_Movie("LibVlc_Movie: Audio track count: %d", audioCount);

    //Debug_Info_Movie("language ref: %d", *p_wc4_language_ref);
    std::string language;
    switch (*p_wc4_language_ref) {
    case 0:
        language = "[English]";
        break;
    case 1:
        language = "[German]";
        break;
    case 2:
        language = "[French]";
        break;
    default:
        return;
        break;
    }
    Debug_Info_Movie("LibVlc_Movie: Search for audio: %s", language.c_str());

#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
    for (size_t i = 0; i < audioCount; i++) {
        libvlc_media_track_t* p_track = libvlc_media_tracklist_at(p_audio_tracklist, i);
        std::string name = p_track->psz_name;

        if (name.find(language) != std::string::npos) {
            Debug_Info_Movie("LibVlc_Movie: Audio track selected ID: %d, name: %s", p_track->i_id, name.c_str());
            libvlc_media_player_select_track(mediaPlayer, p_track);
        }

    }
    libvlc_media_tracklist_delete(p_audio_tracklist);
#else
    std::vector <VLC::TrackDescription> trackDescriptions = mediaPlayer.audioTrackDescription();
    for (size_t i = 0; i < trackDescriptions.size(); i++) {
        VLC::TrackDescription description = trackDescriptions[i];
        //find a audio track description containing matching language text.
        if (description.name().find(language) != std::string::npos) {
            Debug_Info_Movie("LibVlc_Movie: Audio track selected ID: %d, name: %s", description.id(), description.name().c_str());
            mediaPlayer.setAudioTrack(description.id());
        }
    }
#endif

}


//__________________________________________
bool LibVlc_Movie::InitialiseForPlay_Start() {
    if (initialised_for_play)
        return true;
    if (isPlaying) {
        Debug_Info_Movie("LibVlc_Movie: InitialiseForPlay_Start, Play still initialising - cut short: %s", path.c_str());
        InitialiseForPlay_End();
        return true;
    }
    Debug_Info_Movie("LibVlc_Movie: InitialiseForPlay_Start initialising...: %s", path.c_str());
    SetMedia(path);
    isPlaying = mediaPlayer.play();
    if (!isPlaying)
        isError = true;
    if (isError)
        return false;
    //subtitles can not be setup until media is actually playing - on_play() function called.
    while (!is_vlc_playing && !isError) {
        //Debug_Info_Movie("LibVlc_Movie: InitialiseForPlay_Start WAIT");
        Sleep(0);
    }
    Initialise_Subtitles();
    Initialise_Audio();
    while (isPlaying)//play untill InitialiseForPlay_End called.
        Sleep(0);
    return true;
}


//________________________________________
void LibVlc_Movie::InitialiseForPlay_End() {

    Debug_Info_Movie("LibVlc_Movie: InitialiseForPlay_End: %s", path.c_str());
    if (!initialised_for_play) {
        initialised_for_play = true;
        Debug_Info_Movie("LibVlc_Movie: InitialiseForPlay_End - true: %s", path.c_str());
        mediaPlayer.setPause(true);
        isPlaying = false;
    }
}


// For In-flight IFF files modified to use an alphabetical letter value to differentiate each scene. These letters relate to those appended to the name of each individual scene movie file.
// The file name specified in these IFF's "PROF\RADI\FMV " section, should have no file extension. Each scene is numbered correlating with letters in the alphabet starting with 0=a.
// These values are stored in place of the time-code duration value found in unmodified IFF's "PROF\RADI\MSGS" section.
//_________________________________________________________________________________________________________
LibVlc_MovieInflight::LibVlc_MovieInflight(const char* file_name, RECT* p_rc_dest_unscaled, DWORD appendix) {
    libvlc_movieinflight_initialise();

    char movie_name[16]{ 0 };

    strncpy_s(movie_name, _countof(movie_name), file_name, _countof(movie_name) - 1);
    
    char* ext = strrchr(movie_name, '.');
    if (ext) {
        Debug_Info_Error("LibVlc_MovieInflight: file_name should NOT have an extension: %s", movie_name);
        isError = true;
    }

    char* c = movie_name;
    while (*c) {
        *c = tolower(*c);
        c++;
    }

    if (p_rc_dest_unscaled) {
        CopyRect(&rc_dest_unscaled, p_rc_dest_unscaled);
        if (!surface_bg && inflight_display_aspect_type == SCALE_TYPE::fit) {
            LONG bg_x = rc_dest_unscaled.left - 1;
            if (bg_x < 0)
                bg_x = 0;
            LONG bg_y = rc_dest_unscaled.top - 1;
            if (bg_y < 0)
                bg_y = 0;
            DWORD bg_width = rc_dest_unscaled.right - rc_dest_unscaled.left + 1 + 2;
            DWORD bg_height = rc_dest_unscaled.bottom - rc_dest_unscaled.top + 1 + 2;

            surface_bg = new DrawSurface8_RT(0, 0, bg_width, bg_height, 32, inflight_cockpit_bg_colour_argb, true, 255);
            if (!ConfigReadInt(L"MAIN", L"ENABLE_LINEAR_UPSCALING_COCKPIT_HUD", CONFIG_MAIN_ENABLE_LINEAR_UPSCALING_COCKPIT_HUD))
                surface_bg->Set_Default_SamplerState(pd3dPS_SamplerState_Point);
            //Copy cockpit\hud background to draw behind movie. Falling back on inflight_cockpit_bg_colour_argb if starting in HUD view and background is the mask colour.
            BYTE* pSurface = nullptr;
            LONG pitch = 0;
            if (surface_bg->Lock((VOID**)&pSurface, &pitch) == S_OK) {
                DWORD m_width = (**pp_wc4_db_game_main).rc.right - (**pp_wc4_db_game_main).rc.left + 1;
                DWORD m_height = (**pp_wc4_db_game_main).rc.bottom - (**pp_wc4_db_game_main).rc.top + 1;
                BYTE* fBuff = (**pp_wc4_db_game_main).db->buff + m_width * bg_y + bg_x;

                for (UINT y = 0; y < bg_height; y++) {
                    memcpy(pSurface, fBuff, bg_width);
                    fBuff += m_width;
                    pSurface += pitch;
                }
                surface_bg->Unlock();
            }
        }
    }
    else {
        Debug_Info_Error("LibVlc_MovieInflight: destination Rect not set: %s", movie_name);
        isError = true;
    }

    if (!Create_Movie_Path_Inflight(movie_name, appendix, &path)) {
        Debug_Info_Movie("LibVlc_MovieInflight: Create Movie Path Not Found: %s , %s", movie_name, path.c_str());
        isError = true;
    }
    else
        Debug_Info_Movie("LibVlc_MovieInflight: Created Movie Path: %s", path.c_str());

}


//_______________________________________________________________________________________________________________________________________
LibVlc_MovieInflight::LibVlc_MovieInflight(const char* file_name, RECT* p_rc_dest_unscaled, DWORD start_frame_30, DWORD length_frames_30) {
    libvlc_movieinflight_initialise();


    time_ms_start = (libvlc_time_t)start_frame_30 * 1000 / 30;
    time_ms_length = (libvlc_time_t)length_frames_30 * 1000 / 30;
    play_end_time_in_ticks.QuadPart = 0;

    char movie_name[16]{ 0 };

    strncpy_s(movie_name, _countof(movie_name), file_name, _countof(movie_name) - 1);
    char* ext = strrchr(movie_name, '.');
    if (ext)
        *ext = '\0';
    else {
        Debug_Info_Error("LibVlc_MovieInflight: file_name SHOULD have an extension: %s", movie_name);
        isError = true;
    }
    char* c = movie_name;
    while (*c) {
        *c = tolower(*c);
        c++;
    }

    if (p_rc_dest_unscaled) {
        CopyRect(&rc_dest_unscaled, p_rc_dest_unscaled);
        if (!surface_bg && inflight_display_aspect_type == SCALE_TYPE::fit) {
            LONG bg_x = rc_dest_unscaled.left - 1;
            if (bg_x < 0)
                bg_x = 0;
            LONG bg_y = rc_dest_unscaled.top - 1;
            if (bg_y < 0)
                bg_y = 0;
            DWORD bg_width = rc_dest_unscaled.right - rc_dest_unscaled.left + 1 + 2;
            DWORD bg_height = rc_dest_unscaled.bottom - rc_dest_unscaled.top + 1 + 2;

            surface_bg = new DrawSurface8_RT(0, 0, bg_width, bg_height, 32, inflight_cockpit_bg_colour_argb, true, 255);
            if (!ConfigReadInt(L"MAIN", L"ENABLE_LINEAR_UPSCALING_COCKPIT_HUD", CONFIG_MAIN_ENABLE_LINEAR_UPSCALING_COCKPIT_HUD))
                surface_bg->Set_Default_SamplerState(pd3dPS_SamplerState_Point);
            //Copy cockpit\hud background to draw behind movie. Falling back on inflight_cockpit_bg_colour_argb if starting in HUD view and background is the mask colour.
            BYTE* pSurface = nullptr;
            LONG pitch = 0;
            if (surface_bg->Lock((VOID**)&pSurface, &pitch) == S_OK) {
                DWORD m_width = (**pp_wc4_db_game_main).rc.right - (**pp_wc4_db_game_main).rc.left + 1;
                DWORD m_height = (**pp_wc4_db_game_main).rc.bottom - (**pp_wc4_db_game_main).rc.top + 1;
                BYTE* fBuff = (**pp_wc4_db_game_main).db->buff + m_width * bg_y + bg_x;

                for (UINT y = 0; y < bg_height; y++) {
                    memcpy(pSurface, fBuff, bg_width);
                    fBuff += m_width;
                    pSurface += pitch;
                }
                surface_bg->Unlock();
            }
        }
    }
    else {
        Debug_Info_Error("LibVlc_MovieInflight: destination Rect not set: %s", movie_name);
        isError = true;
    }

    if (!Create_Movie_Path_Inflight(movie_name, -1, &path)) {
        Debug_Info_Movie("LibVlc_MovieInflight: Create Movie Path Not Found: %s , %s", movie_name, path.c_str());
        isError = true;
    }
    else
        Debug_Info_Movie("LibVlc_MovieInflight: Created Movie Path: %s", path.c_str());

}


//__________________________________________________________
void LibVlc_MovieInflight::libvlc_movieinflight_initialise() {
    Set_Movie_Settings();
    isPlaying = false;
    hasPlayed = false;
    isError = false;
    has_audio = false;

    surface = nullptr;
    surface_bg = nullptr;
    play_setup_start = false;
    play_setup_complete = false;

    position = 0;
    paused = false;
    is_vlc_playing = false;

    rc_dest_unscaled = { 0,0,0,0 };

    time_ms_start = 0;
    time_ms_length = 0;
    play_end_time_in_ticks.QuadPart = 0;

    using namespace std::placeholders; // for `_1`

    mediaPlayer.eventManager().onPlaying(std::bind(&LibVlc_MovieInflight::on_play, this));
    mediaPlayer.eventManager().onStopped(std::bind(&LibVlc_MovieInflight::on_stopped, this));

    mediaPlayer.eventManager().onEncounteredError(std::bind(&LibVlc_MovieInflight::on_encountered_error, this));
    mediaPlayer.eventManager().onTimeChanged(std::bind(&LibVlc_MovieInflight::on_time_changed, this, _1));

    mediaPlayer.eventManager().onBuffering(std::bind(&LibVlc_MovieInflight::on_buffering, this, _1));
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
    mediaPlayer.eventManager().onStopping(std::bind(&LibVlc_MovieInflight::on_end_reached, this));
#else
    mediaPlayer.eventManager().onEndReached(std::bind(&LibVlc_MovieInflight::on_end_reached, this));
#endif


    mediaPlayer.setVideoCallbacks(std::bind(&LibVlc_MovieInflight::lock, this, _1), std::bind(&LibVlc_MovieInflight::unlock, this, _1, _2), std::bind(&LibVlc_MovieInflight::display, this, _1));
    mediaPlayer.setVideoFormatCallbacks(std::bind(&LibVlc_MovieInflight::format, this, _1, _2, _3, _4, _5), std::bind(&LibVlc_MovieInflight::cleanup, this));

    path.clear();
}


//______________________________________________
void LibVlc_MovieInflight::initialise_for_play() {
    // these need to be set once playback has started
    if (play_setup_start && !play_setup_complete) {
        if (time_ms_length) {
            QueryPerformanceCounter(&play_end_time_in_ticks);
            play_end_time_in_ticks.QuadPart += time_ms_length * p_wc4_frequency->QuadPart / 1000LL;
        }
        play_setup_complete = true;
    }
}


//_______________________________
bool LibVlc_MovieInflight::Play() {
    if (isError)
        isPlaying = false;
    else {
        //Debug_Info("LibVlc_MovieInflight: Play Start");
        SetMedia(path);
        isPlaying = mediaPlayer.play();
        if (!isPlaying)
            isError = true;
        if (isError)
            return false;
        //subtitles can not be setup until media is actually playing - on_play() function called.
        while (!is_vlc_playing && !isError) {
            //Debug_Info_Movie("LibVlc_Movie: InitialiseForPlay_Start WAIT");
            Sleep(0);
        }
        //disable subtitles, we don't want subs overlayed on the inflight video.
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
        libvlc_media_player_unselect_track_type(mediaPlayer, libvlc_track_type_t::libvlc_track_text);
#else
        mediaPlayer.setSpu(-1);
#endif
        //setup audio
        if (!inflight_use_audio_from_file_if_present)
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
            libvlc_media_player_unselect_track_type(mediaPlayer, libvlc_track_type_t::libvlc_track_audio);
#else
            mediaPlayer.setAudioTrack(-1);
#endif

#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
        if(libvlc_media_player_get_selected_track(mediaPlayer, libvlc_track_type_t::libvlc_track_audio) != nullptr) {
#else
        if (mediaPlayer.audioTrack() != -1) {
#endif
            Initialise_Audio();
            Debug_Info_Movie("initialise_for_play: HD movie HAS AUDIO");
            has_audio = true;
        }
        //set the movie start time
        if (time_ms_start) {
            if (mediaPlayer.length() <= time_ms_start)
                time_ms_start = mediaPlayer.length() - 1;
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
            mediaPlayer.setTime(time_ms_start, false);
#else
            mediaPlayer.setTime(time_ms_start);
#endif
        }
    }
    //Debug_Info("LibVlc_MovieInflight: Play END");
    return isPlaying;
}


//___________________________________________________
void LibVlc_MovieInflight::SetMedia(std::string path) {
    VLC::Media media;

#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
    media = VLC::Media(path, VLC::Media::FromPath);
#else
    Debug_Info_Movie("LibVlc_Movie: SetMedia: %s", path.c_str());
    media = VLC::Media(vlc_instance, path, VLC::Media::FromPath);

#endif
    mediaPlayer.setMedia(media);

}


//___________________________________________
void LibVlc_MovieInflight::Initialise_Audio() {

#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
    VLC::MediaPtr media_p = mediaPlayer.media();
    //libvlc_media_parse_request(vlc_instance, *media_p, libvlc_media_parse_local, -1);
    libvlc_media_tracklist_t* p_audio_tracklist = libvlc_media_player_get_tracklist(mediaPlayer, libvlc_track_type_t::libvlc_track_audio, false);
    size_t audioCount = libvlc_media_tracklist_count(p_audio_tracklist);
#else
    size_t audioCount = mediaPlayer.audioTrackCount();
#endif
    Debug_Info_Movie("LibVlc_Movie: Audio track count: %d", audioCount);

    //Debug_Info_Movie("language ref: %d", *p_wc4_language_ref);
    std::string language;
    switch (*p_wc4_language_ref) {
    case 0:
        language = "[English]";
        break;
    case 1:
        language = "[German]";
        break;
    case 2:
        language = "[French]";
        break;
    default:
        return;
        break;
    }
    Debug_Info_Movie("LibVlc_Movie: Search for audio: %s", language.c_str());

#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
    for (size_t i = 0; i < audioCount; i++) {
        libvlc_media_track_t* p_track = libvlc_media_tracklist_at(p_audio_tracklist, i);
        std::string name = p_track->psz_name;

        if (name.find(language) != std::string::npos) {
            Debug_Info_Movie("LibVlc_Movie: Audio track selected ID: %d, name: %s", p_track->i_id, name.c_str());
            libvlc_media_player_select_track(mediaPlayer, p_track);
        }

    }
    libvlc_media_tracklist_delete(p_audio_tracklist);
#else
    std::vector <VLC::TrackDescription> trackDescriptions = mediaPlayer.audioTrackDescription();
    for (size_t i = 0; i < trackDescriptions.size(); i++) {
        VLC::TrackDescription description = trackDescriptions[i];
        //find a audio track description containing matching language text.
        if (description.name().find(language) != std::string::npos) {
            Debug_Info_Movie("LibVlc_Movie: Audio track selected ID: %d, name: %s", description.id(), description.name().c_str());
            mediaPlayer.setAudioTrack(description.id());
        }
    }
#endif

}


//___________________________________________________________________________
void LibVlc_MovieInflight::Update_Display_Dimensions(RECT* p_rc_gui_unscaled) {
    if (p_rc_gui_unscaled) {
        if (p_rc_gui_unscaled->left != rc_dest_unscaled.left || p_rc_gui_unscaled->right != rc_dest_unscaled.right || p_rc_gui_unscaled->top != rc_dest_unscaled.top || p_rc_gui_unscaled->bottom != rc_dest_unscaled.bottom) {
            CopyRect(&rc_dest_unscaled, p_rc_gui_unscaled);
            //Debug_Info("LibVlc_MovieInflight: Update_Display_Dimensions UPDATED: %s", path.c_str());
        }
        else
            return;
    }

    DrawSurface8_RT* pSpace2D_surface = Get_Space2D_Surface();
    float scaleX = 1.0f, scaleY = 1.0f;
    float posX = 1.0f, posY = 1.0f;
    if (pSpace2D_surface) {
        pSpace2D_surface->GetScaledPixelDimensions(&scaleX, &scaleY);
        pSpace2D_surface->GetPosition(&posX, &posY);
    }

    float dest_width = (float)rc_dest_unscaled.right - rc_dest_unscaled.left + 1;
    float dest_height = (float)rc_dest_unscaled.bottom - rc_dest_unscaled.top + 1;
    
    unsigned int int_width = 0;
    unsigned int int_height = 0;
    mediaPlayer.size(0, &int_width, &int_height);

    if (inflight_display_aspect_type == SCALE_TYPE::fill) {
        if (surface) {
            surface->SetPosition(posX + rc_dest_unscaled.left * scaleX, posY + rc_dest_unscaled.top * scaleY);
            surface->SetScale(scaleX, scaleY);
            surface->SetScale(dest_width / (float)int_width * scaleX, dest_height / (float)int_height * scaleX);
        }
        return;
    }

    float movie_width = (float)int_width;
    float movie_height = (float)int_height;
    float movieRO = movie_width / movie_height;
    float destRO = dest_width / (float)dest_height;

    float x = 0;
    float y = 0;
    float width = dest_width;
    float height = dest_height;

    if (movieRO > destRO) {
        x = 0;
        width = (float)dest_width;
        height = (dest_width / movieRO);
        y = ((float)dest_height - height) / 2;
    }
    else {
        y = 0;
        height = (float)dest_height;
        width = dest_height * movieRO;
        x = ((float)dest_width - width) / 2;
    }

    if (surface) {
        surface->SetPosition(posX + (rc_dest_unscaled.left + x) * scaleX, posY + (rc_dest_unscaled.top + y) * scaleY);
        surface->SetScale(width / movie_width * scaleX, height / movie_height * scaleY);
    }
    if (surface_bg) {
        surface_bg->SetPosition(posX + (rc_dest_unscaled.left - 1) * scaleX, posY + (rc_dest_unscaled.top - 1) * scaleY);
        surface_bg->SetScale(scaleX, scaleY);
    }
}


//__________________________________
void LibVlc_MovieInflight::Display() {
    if (!play_setup_complete)//play_counter_started)
        return;
    if (surface_bg && *p_wc4_space_view_type == SPACE_VIEW_TYPE::Cockpit)
        surface_bg->Display();
    if (surface) {
        if (is_inflight_mono_shader_enabled)
            surface->Display(pd3d_PS_Greyscale_Tex_32);
        else
            surface->Display();
    }
}
