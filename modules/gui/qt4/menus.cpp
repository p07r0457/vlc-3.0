/*****************************************************************************
 * menus.cpp : Qt menus
 *****************************************************************************
 * Copyright ( C ) 2006-2007 the VideoLAN team
 * $Id$
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *          Jean-Baptiste Kempf <jb@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc/vlc.h>

#include <vlc_intf_strings.h>

#include "main_interface.hpp"
#include "menus.hpp"
#include "dialogs_provider.hpp"
#include "input_manager.hpp"

#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QActionGroup>
#include <QSignalMapper>
#include <QSystemTrayIcon>

enum
{
    ITEM_NORMAL,
    ITEM_CHECK,
    ITEM_RADIO
};

static QActionGroup *currentGroup;

// Add static entries to menus
void addDPStaticEntry( QMenu *menu,
                       const QString text,
                       const char *help,
                       const char *icon,
                       const char *member,
                       const char *shortcut )
{
    if( !EMPTY_STR( icon ) > 0 )
    {
        if( !EMPTY_STR( shortcut ) > 0 )
            menu->addAction( QIcon( icon ), text, THEDP, member, qtr( shortcut ) );
        else
            menu->addAction( QIcon( icon ), text, THEDP, member );
    }
    else
    {
        if( !EMPTY_STR( shortcut ) > 0 )
            menu->addAction( text, THEDP, member, qtr( shortcut ) );
        else
            menu->addAction( text, THEDP, member );
    }
}

void addMIMStaticEntry( intf_thread_t *p_intf,
                        QMenu *menu,
                        const QString text,
                        const char *help,
                        const char *icon,
                        const char *member )
{
    if( strlen( icon ) > 0 )
    {
        QAction *action = menu->addAction( text, THEMIM,  member );
        action->setIcon( QIcon( icon ) );
    }
    else
    {
        menu->addAction( text, THEMIM, member );
    }
}

/*****************************************************************************
 * Definitions of variables for the dynamic menus
 *****************************************************************************/
#define PUSH_VAR( var ) varnames.push_back( var ); \
    objects.push_back( p_object->i_object_id )

#define PUSH_SEPARATOR if( objects.size() != i_last_separator ) { \
    objects.push_back( 0 ); varnames.push_back( "" ); \
    i_last_separator = objects.size(); }

static int InputAutoMenuBuilder( vlc_object_t *p_object,
        vector<int> &objects,
        vector<const char *> &varnames )
{
    PUSH_VAR( "bookmark" );
    PUSH_VAR( "title" );
    PUSH_VAR( "chapter" );
    PUSH_VAR( "program" );
    PUSH_VAR( "navigation" );
    PUSH_VAR( "dvd_menus" );
    return VLC_SUCCESS;
}

static int VideoAutoMenuBuilder( vlc_object_t *p_object,
        vector<int> &objects,
        vector<const char *> &varnames )
{
    PUSH_VAR( "fullscreen" );
    PUSH_VAR( "zoom" );
    PUSH_VAR( "deinterlace" );
    PUSH_VAR( "aspect-ratio" );
    PUSH_VAR( "crop" );
    PUSH_VAR( "video-on-top" );
    PUSH_VAR( "directx-wallpaper" );
    PUSH_VAR( "video-snapshot" );

    vlc_object_t *p_dec_obj = ( vlc_object_t * )vlc_object_find( p_object,
            VLC_OBJECT_DECODER,
            FIND_PARENT );
    if( p_dec_obj != NULL )
    {
        vlc_object_t *p_object = p_dec_obj;
        PUSH_VAR( "ffmpeg-pp-q" );
        vlc_object_release( p_dec_obj );
    }
    return VLC_SUCCESS;
}

static int AudioAutoMenuBuilder( vlc_object_t *p_object,
        vector<int> &objects,
        vector<const char *> &varnames )
{
    PUSH_VAR( "audio-device" );
    PUSH_VAR( "audio-channels" );
    PUSH_VAR( "visual" );
    PUSH_VAR( "equalizer" );
    return VLC_SUCCESS;
}

/*****************************************************************************
 * All normal menus
 * Simple Code
 *****************************************************************************/

#define BAR_ADD( func, title ) { \
    QMenu *menu = func; menu->setTitle( title ); bar->addMenu( menu ); }

#define BAR_DADD( func, title, id ) { \
    QMenu *menu = func; menu->setTitle( title ); bar->addMenu( menu ); \
    MenuFunc *f = new MenuFunc( menu, id ); \
    CONNECT( menu, aboutToShow(), THEDP->menusUpdateMapper, map() ); \
    THEDP->menusUpdateMapper->setMapping( menu, f ); }

/**
 * Main Menu Bar Creation
 **/
void QVLCMenu::createMenuBar( MainInterface *mi,
                              intf_thread_t *p_intf,
                              bool visual_selector_enabled )
{
    QMenuBar *bar = mi->menuBar();
    BAR_ADD( FileMenu(), qtr( "&Media" ) );
    BAR_ADD( PlaylistMenu( p_intf, mi ), qtr( "&Playlist" ) );
    BAR_ADD( ToolsMenu( p_intf, mi, visual_selector_enabled, true ),
             qtr( "&Tools" ) );
    BAR_DADD( AudioMenu( p_intf, NULL ), qtr( "&Audio" ), 2 );
    BAR_DADD( VideoMenu( p_intf, NULL ), qtr( "&Video" ), 1 );
    BAR_DADD( NavigMenu( p_intf, NULL ), qtr( "&Playback" ), 3 );

    BAR_ADD( HelpMenu(), qtr( "&Help" ) );
}
#undef BAR_ADD
#undef BAR_DADD

/**
 * Media ( File ) Menu
 * Opening, streaming and quit
 **/
QMenu *QVLCMenu::FileMenu()
{
    QMenu *menu = new QMenu();

    addDPStaticEntry( menu, qtr( "&Open File..." ), "",
        ":/pixmaps/file-asym_16px.png", SLOT( openFileDialog() ), "Ctrl+O" );
    addDPStaticEntry( menu, qtr( I_OPEN_FOLDER ), "",
        ":/pixmaps/folder-grey_16px.png", SLOT( PLAppendDir() ), "Ctrl+F" );
    addDPStaticEntry( menu, qtr( "Open &Disc..." ), "",
        ":/pixmaps/disc_16px.png", SLOT( openDiscDialog() ), "Ctrl+D" );
    addDPStaticEntry( menu, qtr( "Open &Network..." ), "",
        ":/pixmaps/network_16px.png", SLOT( openNetDialog() ), "Ctrl+N" );
    addDPStaticEntry( menu, qtr( "Open &Capture Device..." ), "",
        ":/pixmaps/capture-card_16px.png", SLOT( openCaptureDialog() ),
        "Ctrl+C" );
    menu->addSeparator();

    addDPStaticEntry( menu, qtr( "&Streaming..." ), "",
        ":/pixmaps/menus_stream_16px.png", SLOT( openThenStreamingDialogs() ),
        "Ctrl+S" );
    addDPStaticEntry( menu, qtr( "Conve&rt / Save..." ), "", "",
        SLOT( openThenTranscodingDialogs() ), "Ctrl+R" );
    menu->addSeparator();

    addDPStaticEntry( menu, qtr( "&Quit" ) , "",
        ":/pixmaps/menus_quit_16px.png", SLOT( quit() ), "Ctrl+Q" );
    return menu;
}

/* Playlist/MediaLibrary Control */
QMenu *QVLCMenu::PlaylistMenu( intf_thread_t *p_intf, MainInterface *mi )
{
    QMenu *menu = new QMenu();
    menu->addMenu( SDMenu( p_intf ) );
    menu->addAction( QIcon( ":/pixmaps/playlist_16px.png" ),
                     qtr( "Show Playlist" ), mi, SLOT( togglePlaylist() ) );
    menu->addSeparator();

    addDPStaticEntry( menu, qtr( I_PL_LOAD ), "", "", SLOT( openAPlaylist() ),
        "Ctrl+X" );
    addDPStaticEntry( menu, qtr( I_PL_SAVE ), "", "", SLOT( saveAPlaylist() ),
        "Ctrl+Y" );
    menu->addSeparator();
    menu->addAction( qtr( "Undock from interface" ), mi,
                     SLOT( undockPlaylist() ), qtr( "Ctrl+U" ) );
    return menu;
}

/**
 * Tools/View Menu
 * This is kept in the same menu for now, but could change if it gets much
 * longer.
 * This menu can be an interface menu but also a right click menu.
 **/
QMenu *QVLCMenu::ToolsMenu( intf_thread_t *p_intf,
                            MainInterface *mi,
                            bool visual_selector_enabled,
                            bool with_intf )
{
    QMenu *menu = new QMenu;
    if( mi )
    {
        menu->addAction( QIcon( ":/pixmaps/playlist_16px.png" ),
                         qtr( "Playlist..." ), mi, SLOT( togglePlaylist() ),
                         qtr( "Ctrl+L" ) );
    }
    addDPStaticEntry( menu, qtr( I_MENU_EXT ), "",
        ":/pixmaps/menus_settings_16px.png", SLOT( extendedDialog() ),
        "Ctrl+E" );

    menu->addSeparator();

    if( with_intf )
    {
        QMenu *intfmenu = InterfacesMenu( p_intf, NULL );
        intfmenu->setTitle( qtr( "Interfaces" ) );
        menu->addMenu( intfmenu );
        menu->addSeparator();
    }
    if( mi )
    {
        /* Minimal View */
        QAction *action=menu->addAction( qtr( "Minimal View..." ), mi,
                SLOT( toggleMinimalView() ), qtr( "Ctrl+H" ) );
        action->setCheckable( true );
        if( mi->getControlsVisibilityStatus() & CONTROLS_VISIBLE )
            action->setChecked( true );

        /* FullScreen View */
        action = menu->addAction( qtr( "Toggle Fullscreen Interface" ), mi,
                SLOT( toggleFullScreen() ), qtr( "F11" ) );

        /* Advanced Controls */
        action = menu->addAction( qtr( "Advanced controls" ), mi,
                SLOT( toggleAdvanced() ) );
        action->setCheckable( true );
        if( mi->getControlsVisibilityStatus() & CONTROLS_ADVANCED )
            action->setChecked( true );
#if 0 /* For Visualisations. Not yet working */
        adv = menu->addAction( qtr( "Visualizations selector" ),
                mi, SLOT( visual() ) );
        adv->setCheckable( true );
        if( visual_selector_enabled ) adv->setChecked( true );
#endif
    }

    menu->addSeparator();

    addDPStaticEntry( menu, qtr( I_MENU_MSG ), "",
        ":/pixmaps/menus_messages_16px.png", SLOT( messagesDialog() ),
        "Ctrl+M" );
    addDPStaticEntry( menu, qtr( I_MENU_INFO ) , "", "",
        SLOT( mediaInfoDialog() ), "Ctrl+I" );
    addDPStaticEntry( menu, qtr( I_MENU_CODECINFO ) , "",
        ":/pixmaps/menus_info_16px.png", SLOT( mediaCodecDialog() ), "Ctrl+J" );
    addDPStaticEntry( menu, qtr( I_MENU_BOOKMARK ), "","",
                      SLOT( bookmarksDialog() ), "Ctrl+B" );
#ifdef ENABLE_VLM
    addDPStaticEntry( menu, qtr( I_MENU_VLM ), "", "", SLOT( vlmDialog() ),
        "Ctrl+W" );
#endif

    menu->addSeparator();
    addDPStaticEntry( menu, qtr( "Preferences..." ), "",
        ":/pixmaps/menus_preferences_16px.png", SLOT( prefsDialog() ), "Ctrl+P" );
    return menu;
}

/**
 * Interface Sub-Menu, to list extras interface and skins
 **/
QMenu *QVLCMenu::InterfacesMenu( intf_thread_t *p_intf, QMenu *current )
{
    vector<int> objects;
    vector<const char *> varnames;
    /** \todo add "switch to XXX" */
    varnames.push_back( "intf-add" );
    objects.push_back( p_intf->i_object_id );

    QMenu *menu = Populate( p_intf, current, varnames, objects );

    CONNECT( menu, aboutToShow(), THEDP->menusUpdateMapper, map() );
    THEDP->menusUpdateMapper->setMapping( menu, 4 );
    return menu;
}

/**
 * Main Audio Menu
 */
QMenu *QVLCMenu::AudioMenu( intf_thread_t *p_intf, QMenu * current )
{
    vector<int> objects;
    vector<const char *> varnames;

    vlc_object_t *p_object = ( vlc_object_t * )vlc_object_find( p_intf,
            VLC_OBJECT_INPUT, FIND_ANYWHERE );
    if( p_object != NULL )
    {
        PUSH_VAR( "audio-es" );
        vlc_object_release( p_object );
    }

    p_object = ( vlc_object_t * )vlc_object_find( p_intf, VLC_OBJECT_AOUT,
            FIND_ANYWHERE );
    if( p_object )
    {
        AudioAutoMenuBuilder( p_object, objects, varnames );
        vlc_object_release( p_object );
    }
    return Populate( p_intf, current, varnames, objects );
}

/**
 * Main Video Menu
 * Subtitles are part of Video.
 **/
QMenu *QVLCMenu::VideoMenu( intf_thread_t *p_intf, QMenu *current )
{
    vlc_object_t *p_object;
    vector<int> objects;
    vector<const char *> varnames;

    p_object = ( vlc_object_t * )vlc_object_find( p_intf, VLC_OBJECT_INPUT,
            FIND_ANYWHERE );
    if( p_object != NULL )
    {
        PUSH_VAR( "video-es" );
        PUSH_VAR( "spu-es" );
        vlc_object_release( p_object );
    }

    p_object = ( vlc_object_t * )vlc_object_find( p_intf, VLC_OBJECT_VOUT,
            FIND_ANYWHERE );
    if( p_object != NULL )
    {
        VideoAutoMenuBuilder( p_object, objects, varnames );
        vlc_object_release( p_object );
    }
    return Populate( p_intf, current, varnames, objects );
}

/**
 * Navigation Menu
 * For DVD, MP4, MOV and other chapter based format
 **/
QMenu *QVLCMenu::NavigMenu( intf_thread_t *p_intf, QMenu *navMenu )
{
    vlc_object_t *p_object;
    vector<int> objects;
    vector<const char *> varnames;

    p_object = ( vlc_object_t * )vlc_object_find( p_intf, VLC_OBJECT_INPUT,
            FIND_ANYWHERE );
    if( p_object != NULL )
    {
        InputAutoMenuBuilder(  p_object, objects, varnames );
        PUSH_VAR( "prev-title" ); PUSH_VAR ( "next-title" );
        PUSH_VAR( "prev-chapter" ); PUSH_VAR( "next-chapter" );
        vlc_object_release( p_object );
    }
    navMenu = new QMenu();
    addDPStaticEntry( navMenu, qtr( I_MENU_GOTOTIME ), "","",
        SLOT( gotoTimeDialog() ), "Ctrl+T" );
    navMenu->addSeparator();
    return Populate( p_intf, navMenu, varnames, objects, true );
}

/**
 * Service Discovery SubMenu
 **/
QMenu *QVLCMenu::SDMenu( intf_thread_t *p_intf )
{
    QMenu *menu = new QMenu();
    menu->setTitle( qtr( I_PL_SD ) );
    char **ppsz_longnames;
    char **ppsz_names = services_discovery_GetServicesNames( p_intf,
                                                             &ppsz_longnames );
    if( !ppsz_names )
        return menu;

    char **ppsz_name = ppsz_names, **ppsz_longname = ppsz_longnames;
    for( ; *ppsz_name; ppsz_name++, ppsz_longname++ )
    {
        QAction *a = new QAction( qfu( *ppsz_longname ), menu );
        a->setCheckable( true );
        if( playlist_IsServicesDiscoveryLoaded( THEPL, *ppsz_name ) )
            a->setChecked( true );
        CONNECT( a , triggered(), THEDP->SDMapper, map() );
        THEDP->SDMapper->setMapping( a, QString( *ppsz_name ) );
        menu->addAction( a );

        if( !strcmp( *ppsz_name, "podcast" ) )
        {
            QAction *b = new QAction( qfu( "Configure podcasts..." ), menu );
            //b->setEnabled( a->isChecked() );
            menu->addAction( b );
            CONNECT( b, triggered(), THEDP, podcastConfigureDialog() );
        }
        free( *ppsz_name );
        free( *ppsz_longname );
    }
    free( ppsz_names );
    free( ppsz_longnames );
    return menu;
}
/**
 * Help/About Menu
**/
QMenu *QVLCMenu::HelpMenu()
{
    QMenu *menu = new QMenu();
    addDPStaticEntry( menu, qtr( "Help..." ) , "",
        ":/pixmaps/menus_help_16px.png", SLOT( helpDialog() ), "F1" );
#ifdef UPDATE_CHECK
    addDPStaticEntry( menu, qtr( "Check for updates..." ) , "", "", SLOT( updateDialog() ), "");
#endif
    menu->addSeparator();
    addDPStaticEntry( menu, qtr( I_MENU_ABOUT ), "", "", SLOT( aboutDialog() ),
        "Ctrl+F1" );
    return menu;
}


/*****************************************************************************
 * Popup menus - Right Click menus                                           *
 *****************************************************************************/
#define POPUP_BOILERPLATE \
    unsigned int i_last_separator = 0; \
    vector<int> objects; \
    vector<const char *> varnames; \
    input_thread_t *p_input = THEMIM->getInput();

#define CREATE_POPUP \
    Populate( p_intf, menu, varnames, objects ); \
    p_intf->p_sys->p_popup_menu = menu; \
    menu->popup( QCursor::pos() ); \
    p_intf->p_sys->p_popup_menu = NULL; \
    i_last_separator = 0;

void QVLCMenu::PopupMenuControlEntries( QMenu *menu,
                                        intf_thread_t *p_intf,
                                        input_thread_t *p_input )
{
    if( p_input )
    {
        vlc_value_t val;
        var_Get( p_input, "state", &val );
        if( val.i_int == PLAYING_S )
            addMIMStaticEntry( p_intf, menu, qtr( "Pause" ), "",
                    ":/pixmaps/pause_16px.png", SLOT( togglePlayPause() ) );
        else
            addMIMStaticEntry( p_intf, menu, qtr( "Play" ), "",
                    ":/pixmaps/play_16px.png", SLOT( togglePlayPause() ) );
    }
    else if( THEPL->items.i_size && THEPL->i_enabled )
        addMIMStaticEntry( p_intf, menu, qtr( "Play" ), "",
                ":/pixmaps/play_16px.png", SLOT( togglePlayPause() ) );

    addMIMStaticEntry( p_intf, menu, qtr( "Stop" ), "",
            ":/pixmaps/stop_16px.png", SLOT( stop() ) );
    addMIMStaticEntry( p_intf, menu, qtr( "Previous" ), "",
            ":/pixmaps/previous_16px.png", SLOT( prev() ) );
    addMIMStaticEntry( p_intf, menu, qtr( "Next" ), "",
            ":/pixmaps/next_16px.png", SLOT( next() ) );
    }

void QVLCMenu::PopupMenuStaticEntries( intf_thread_t *p_intf, QMenu *menu )
{
    QMenu *toolsmenu = ToolsMenu( p_intf, NULL, false, true );
    toolsmenu->setTitle( qtr( "Tools" ) );
    menu->addMenu( toolsmenu );

    QMenu *openmenu = new QMenu( qtr( "Open" ) );
    openmenu->addAction( qtr( "Open &File..." ), THEDP,
                         SLOT( openFileDialog() ) );
    openmenu->addAction( qtr( "Open &Disc..." ), THEDP,
                         SLOT( openDiscDialog() ) );
    openmenu->addAction( qtr( "Open &Network..." ), THEDP,
                         SLOT( openNetDialog() ) );
    openmenu->addAction( qtr( "Open &Capture Device..." ), THEDP,
                         SLOT( openCaptureDialog() ) );
    menu->addMenu( openmenu );

    menu->addSeparator();
    QMenu *helpmenu = HelpMenu();
    helpmenu->setTitle( qtr( "Help" ) );
    menu->addMenu( helpmenu );

    addDPStaticEntry( menu, qtr( "Quit" ), "", "", SLOT( quit() ) , "Ctrl+Q" );
}

/* Video Tracks and Subtitles tracks */
void QVLCMenu::VideoPopupMenu( intf_thread_t *p_intf )
{
    POPUP_BOILERPLATE;
    if( p_input )
    {
        vlc_object_yield( p_input );
        varnames.push_back( "video-es" );
        objects.push_back( p_input->i_object_id );
        varnames.push_back( "spu-es" );
        objects.push_back( p_input->i_object_id );
        vlc_object_t *p_vout = ( vlc_object_t * )vlc_object_find( p_input,
                VLC_OBJECT_VOUT, FIND_CHILD );
        if( p_vout )
        {
            VideoAutoMenuBuilder( p_vout, objects, varnames );
            vlc_object_release( p_vout );
        }
        vlc_object_release( p_input );
    }
    QMenu *menu = new QMenu();
    CREATE_POPUP;
}

/* Audio Tracks */
void QVLCMenu::AudioPopupMenu( intf_thread_t *p_intf )
{
    POPUP_BOILERPLATE;
    if( p_input )
    {
        vlc_object_yield( p_input );
        varnames.push_back( "audio-es" );
        objects.push_back( p_input->i_object_id );
        vlc_object_t *p_aout = ( vlc_object_t * )vlc_object_find( p_input,
                VLC_OBJECT_AOUT, FIND_ANYWHERE );
        if( p_aout )
        {
            AudioAutoMenuBuilder( p_aout, objects, varnames );
            vlc_object_release( p_aout );
        }
        vlc_object_release( p_input );
    }
    QMenu *menu = new QMenu();
    CREATE_POPUP;
}

/* Navigation stuff, and general menus ( open ) */
void QVLCMenu::MiscPopupMenu( intf_thread_t *p_intf )
{
    vlc_value_t val;
    POPUP_BOILERPLATE;

    if( p_input )
    {
        vlc_object_yield( p_input );
        varnames.push_back( "audio-es" );
        InputAutoMenuBuilder( VLC_OBJECT( p_input ), objects, varnames );
        PUSH_SEPARATOR;
    }

    QMenu *menu = new QMenu();
    Populate( p_intf, menu, varnames, objects );

    menu->addSeparator();
    PopupMenuControlEntries( menu, p_intf, p_input );

    menu->addSeparator();
    PopupMenuStaticEntries( p_intf, menu );

    p_intf->p_sys->p_popup_menu = menu;
    menu->popup( QCursor::pos() );
    p_intf->p_sys->p_popup_menu = NULL;
}

/* Main Menu that sticks everything together  */
void QVLCMenu::PopupMenu( intf_thread_t *p_intf, bool show )
{
    if( show )
    {
        // create a  popup if there is none
        if( ! p_intf->p_sys->p_popup_menu )
        {
            POPUP_BOILERPLATE;
            if( p_input )
            {
                vlc_object_yield( p_input );
                InputAutoMenuBuilder( VLC_OBJECT( p_input ), objects, varnames );

                /* Audio menu */
                PUSH_SEPARATOR;
                varnames.push_back( "audio-es" );
                objects.push_back( p_input->i_object_id );
                vlc_object_t *p_aout = ( vlc_object_t * )
                    vlc_object_find( p_input, VLC_OBJECT_AOUT, FIND_ANYWHERE );
                if( p_aout )
                {
                    AudioAutoMenuBuilder( p_aout, objects, varnames );
                    vlc_object_release( p_aout );
                }

                /* Video menu */
                PUSH_SEPARATOR;
                varnames.push_back( "video-es" );
                objects.push_back( p_input->i_object_id );
                varnames.push_back( "spu-es" );
                objects.push_back( p_input->i_object_id );
                vlc_object_t *p_vout = ( vlc_object_t * )
                    vlc_object_find( p_input, VLC_OBJECT_VOUT, FIND_CHILD );
                if( p_vout )
                {
                    VideoAutoMenuBuilder( p_vout, objects, varnames );
                    vlc_object_release( p_vout );
                }
            }

            QMenu *menu = new QMenu();
            Populate( p_intf, menu, varnames, objects );
            menu->addSeparator();
            PopupMenuControlEntries( menu, p_intf, p_input );
            menu->addSeparator();
            PopupMenuStaticEntries( p_intf, menu );

            p_intf->p_sys->p_popup_menu = menu;
        }
        p_intf->p_sys->p_popup_menu->popup( QCursor::pos() );
    }
    else
    {
        // destroy popup if there is one
        delete p_intf->p_sys->p_popup_menu;
        p_intf->p_sys->p_popup_menu = NULL;
    }
}

/************************************************************************
 * Systray Menu                                                         *
 ************************************************************************/

void QVLCMenu::updateSystrayMenu( MainInterface *mi,
                                  intf_thread_t *p_intf,
                                  bool b_force_visible )
{
    POPUP_BOILERPLATE;

    /* Get the systray menu and clean it */
    QMenu *sysMenu = mi->getSysTrayMenu();
    sysMenu->clear();

    /* Hide / Show VLC and cone */
    if( mi->isVisible() || b_force_visible )
    {
        sysMenu->addAction( QIcon( ":/vlc16.png" ),
                qtr( "Hide VLC media player in taskbar" ), mi,
                SLOT( toggleUpdateSystrayMenu() ) );
    }
    else
    {
        sysMenu->addAction( QIcon( ":/vlc16.png" ),
                qtr( "Show VLC media player" ), mi,
                SLOT( toggleUpdateSystrayMenu() ) );
    }

    sysMenu->addSeparator();
    PopupMenuControlEntries( sysMenu, p_intf, p_input );

    sysMenu->addSeparator();
    addDPStaticEntry( sysMenu, qtr( "&Open Media" ), "",
            ":/pixmaps/file-wide_16px.png", SLOT( openFileDialog() ), "" );
    addDPStaticEntry( sysMenu, qtr( "&Quit" ) , "",
        ":/pixmaps/menus_quit_16px.png", SLOT( quit() ), "" );

    /* Set the menu */
    mi->getSysTray()->setContextMenu( sysMenu );
}

#undef PUSH_VAR
#undef PUSH_SEPARATOR


/*************************************************************************
 * Builders for automenus
 *************************************************************************/
QMenu * QVLCMenu::Populate( intf_thread_t *p_intf,
                            QMenu *current,
                            vector< const char *> & varnames,
                            vector<int> & objects,
                            bool append )
{
    QMenu *menu = current;
    if( !menu )
        menu = new QMenu();
    else if( !append )
        menu->clear();

    currentGroup = NULL;

    vlc_object_t *p_object;
    vlc_bool_t b_section_empty = VLC_FALSE;
    int i;

#define APPEND_EMPTY { QAction *action = menu->addAction( qtr( "Empty" ) ); \
    action->setEnabled( false ); }

    for( i = 0; i < ( int )objects.size() ; i++ )
    {
        if( !varnames[i] || !*varnames[i] )
        {
            if( b_section_empty )
                APPEND_EMPTY;
            menu->addSeparator();
            b_section_empty = VLC_TRUE;
            continue;
        }

        if( objects[i] == 0 )
        {
            /// \bug What is this ?
            // Append( menu, varnames[i], NULL );
            b_section_empty = VLC_FALSE;
            continue;
        }

        p_object = ( vlc_object_t * )vlc_object_get( objects[i] );
        if( p_object == NULL ) continue;

        b_section_empty = VLC_FALSE;
        /* Ugly specific stuff */
        if( strstr( varnames[i], "intf-add" ) )
            CreateItem( menu, varnames[i], p_object, false );
        else
            CreateItem( menu, varnames[i], p_object, true );
        vlc_object_release( p_object );
    }

    /* Special case for empty menus */
    if( menu->actions().size() == 0 || b_section_empty )
        APPEND_EMPTY

    return menu;
}
#undef APPEND_EMPTY

/*****************************************************************************
 * Private methods.
 *****************************************************************************/

static bool IsMenuEmpty( const char *psz_var,
                         vlc_object_t *p_object,
                         bool b_root = true )
{
    vlc_value_t val, val_list;
    int i_type, i_result, i;

    /* Check the type of the object variable */
    i_type = var_Type( p_object, psz_var );

    /* Check if we want to display the variable */
    if( !( i_type & VLC_VAR_HASCHOICE ) ) return false;

    var_Change( p_object, psz_var, VLC_VAR_CHOICESCOUNT, &val, NULL );
    if( val.i_int == 0 ) return true;

    if( ( i_type & VLC_VAR_TYPE ) != VLC_VAR_VARIABLE )
    {
        if( val.i_int == 1 && b_root ) return true;
        else return false;
    }

    /* Check children variables in case of VLC_VAR_VARIABLE */
    if( var_Change( p_object, psz_var, VLC_VAR_GETLIST, &val_list, NULL ) < 0 )
    {
        return true;
    }

    for( i = 0, i_result = true; i < val_list.p_list->i_count; i++ )
    {
        if( !IsMenuEmpty( val_list.p_list->p_values[i].psz_string,
                    p_object, false ) )
        {
            i_result = false;
            break;
        }
    }

    /* clean up everything */
    var_Change( p_object, psz_var, VLC_VAR_FREELIST, &val_list, NULL );

    return i_result;
}

void QVLCMenu::CreateItem( QMenu *menu, const char *psz_var,
        vlc_object_t *p_object, bool b_submenu )
{
    vlc_value_t val, text;
    int i_type;

    /* Check the type of the object variable */
    i_type = var_Type( p_object, psz_var );

    switch( i_type & VLC_VAR_TYPE )
    {
        case VLC_VAR_VOID:
        case VLC_VAR_BOOL:
        case VLC_VAR_VARIABLE:
        case VLC_VAR_STRING:
        case VLC_VAR_INTEGER:
        case VLC_VAR_FLOAT:
            break;
        default:
            /* Variable doesn't exist or isn't handled */
            return;
    }

    /* Make sure we want to display the variable */
    if( IsMenuEmpty( psz_var, p_object ) )  return;

    /* Get the descriptive name of the variable */
    var_Change( p_object, psz_var, VLC_VAR_GETTEXT, &text, NULL );

    if( i_type & VLC_VAR_HASCHOICE )
    {
        /* Append choices menu */
        if( b_submenu )
        {
            QMenu *submenu = new QMenu();
            submenu->setTitle( qfu( text.psz_string ?
                        text.psz_string : psz_var ) );
            if( CreateChoicesMenu( submenu, psz_var, p_object, true ) == 0 )
                menu->addMenu( submenu );
        }
        else
            CreateChoicesMenu( menu, psz_var, p_object, true );
        FREENULL( text.psz_string );
        return;
    }

#define TEXT_OR_VAR qfu ( text.psz_string ? text.psz_string : psz_var )

    switch( i_type & VLC_VAR_TYPE )
    {
        case VLC_VAR_VOID:
            var_Get( p_object, psz_var, &val );
            CreateAndConnect( menu, psz_var, TEXT_OR_VAR, "", ITEM_NORMAL,
                    p_object->i_object_id, val, i_type );
            break;

        case VLC_VAR_BOOL:
            var_Get( p_object, psz_var, &val );
            val.b_bool = !val.b_bool;
            CreateAndConnect( menu, psz_var, TEXT_OR_VAR, "", ITEM_CHECK,
                    p_object->i_object_id, val, i_type, !val.b_bool );
            break;
    }
    FREENULL( text.psz_string );
}


int QVLCMenu::CreateChoicesMenu( QMenu *submenu, const char *psz_var,
        vlc_object_t *p_object, bool b_root )
{
    vlc_value_t val, val_list, text_list;
    int i_type, i;

    /* Check the type of the object variable */
    i_type = var_Type( p_object, psz_var );

    /* Make sure we want to display the variable */
    if( IsMenuEmpty( psz_var, p_object, b_root ) ) return VLC_EGENERIC;

    switch( i_type & VLC_VAR_TYPE )
    {
        case VLC_VAR_VOID:
        case VLC_VAR_BOOL:
        case VLC_VAR_VARIABLE:
        case VLC_VAR_STRING:
        case VLC_VAR_INTEGER:
        case VLC_VAR_FLOAT:
            break;
        default:
            /* Variable doesn't exist or isn't handled */
            return VLC_EGENERIC;
    }

    if( var_Change( p_object, psz_var, VLC_VAR_GETLIST,
                &val_list, &text_list ) < 0 )
    {
        return VLC_EGENERIC;
    }

#define NORMAL_OR_RADIO i_type & VLC_VAR_ISCOMMAND ? ITEM_NORMAL: ITEM_RADIO
#define NOTCOMMAND !( i_type & VLC_VAR_ISCOMMAND )
#define CURVAL val_list.p_list->p_values[i]
#define CURTEXT text_list.p_list->p_values[i].psz_string

    for( i = 0; i < val_list.p_list->i_count; i++ )
    {
        vlc_value_t another_val;
        QString menutext;
        QMenu *subsubmenu = new QMenu();

        switch( i_type & VLC_VAR_TYPE )
        {
            case VLC_VAR_VARIABLE:
                CreateChoicesMenu( subsubmenu, CURVAL.psz_string, p_object, false );
                subsubmenu->setTitle( qfu( CURTEXT ? CURTEXT :CURVAL.psz_string ) );
                submenu->addMenu( subsubmenu );
                break;

            case VLC_VAR_STRING:
                var_Get( p_object, psz_var, &val );
                another_val.psz_string = strdup( CURVAL.psz_string );
                menutext = qfu( "Add " ) /* If this function is more used, FIX*/
                         + qfu( CURTEXT ? CURTEXT : another_val.psz_string );
                CreateAndConnect( submenu, psz_var, menutext, "", NORMAL_OR_RADIO,
                        p_object->i_object_id, another_val, i_type,
                        NOTCOMMAND && val.psz_string &&
                        !strcmp( val.psz_string, CURVAL.psz_string ) );

                free( val.psz_string );
                break;

            case VLC_VAR_INTEGER:
                var_Get( p_object, psz_var, &val );
                if( CURTEXT ) menutext = qfu( CURTEXT );
                else menutext.sprintf( "%d", CURVAL.i_int );
                CreateAndConnect( submenu, psz_var, menutext, "", NORMAL_OR_RADIO,
                        p_object->i_object_id, CURVAL, i_type,
                        NOTCOMMAND && CURVAL.i_int == val.i_int );
                break;

            case VLC_VAR_FLOAT:
                var_Get( p_object, psz_var, &val );
                if( CURTEXT ) menutext = qfu( CURTEXT );
                else menutext.sprintf( "%.2f", CURVAL.f_float );
                CreateAndConnect( submenu, psz_var, menutext, "", NORMAL_OR_RADIO,
                        p_object->i_object_id, CURVAL, i_type,
                        NOTCOMMAND && CURVAL.f_float == val.f_float );
                break;

            default:
                break;
        }
    }
    currentGroup = NULL;

    /* clean up everything */
    var_Change( p_object, psz_var, VLC_VAR_FREELIST, &val_list, &text_list );

#undef NORMAL_OR_RADIO
#undef NOTCOMMAND
#undef CURVAL
#undef CURTEXT
    return VLC_SUCCESS;
}

void QVLCMenu::CreateAndConnect( QMenu *menu, const char *psz_var,
        QString text, QString help,
        int i_item_type, int i_object_id,
        vlc_value_t val, int i_val_type,
        bool checked )
{
    QAction *action = new QAction( text, menu );
    action->setText( text );
    action->setToolTip( help );

    if( i_item_type == ITEM_CHECK )
    {
        action->setCheckable( true );
    }
    else if( i_item_type == ITEM_RADIO )
    {
        action->setCheckable( true );
        if( !currentGroup )
            currentGroup = new QActionGroup( menu );
        currentGroup->addAction( action );
    }

    if( checked )
    {
        action->setChecked( true );
    }
    MenuItemData *itemData = new MenuItemData( i_object_id, i_val_type,
            val, psz_var );
    CONNECT( action, triggered(), THEDP->menusMapper, map() );
    THEDP->menusMapper->setMapping( action, itemData );
    menu->addAction( action );
}

void QVLCMenu::DoAction( intf_thread_t *p_intf, QObject *data )
{
    MenuItemData *itemData = qobject_cast<MenuItemData *>( data );
    vlc_object_t *p_object = ( vlc_object_t * )vlc_object_get( itemData->i_object_id );
    if( p_object == NULL ) return;

    var_Set( p_object, itemData->psz_var, itemData->val );
    vlc_object_release( p_object );
}

