/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "guiinfo/GUIControlsGUIInfo.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "dialogs/GUIDialogKeyboardGeneric.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogProgress.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUITextBox.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/IGUIContainer.h"
#include "guilib/LocalizeStrings.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "music/dialogs/GUIDialogSongInfo.h"
#include "music/tags/MusicInfoTag.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "settings/Settings.h"
#include "video/VideoInfoTag.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "view/GUIViewState.h"
#include "windows/GUIMediaWindow.h"

#include "guiinfo/GUIInfo.h"
#include "guiinfo/GUIInfoHelper.h"
#include "guiinfo/GUIInfoLabels.h"

using namespace GUIINFO;

CGUIControlsGUIInfo::CGUIControlsGUIInfo()
: m_nextWindowID(WINDOW_INVALID),
  m_prevWindowID(WINDOW_INVALID)
{
}

void CGUIControlsGUIInfo::SetContainerMoving(int id, bool next, bool scrolling)
{
  // magnitude 2 indicates a scroll, sign indicates direction
  m_containerMoves[id] = (next ? 1 : -1) * (scrolling ? 2 : 1);
}

void CGUIControlsGUIInfo::ResetContainerMovingCache()
{
  m_containerMoves.clear();
}

bool CGUIControlsGUIInfo::InitCurrentItem(CFileItem *item)
{
  return false;
}

bool CGUIControlsGUIInfo::GetLabel(std::string& value, const CFileItem *item, int contextWindow, const GUIInfo &info, std::string *fallback) const
{
  CGUIWindow *window = CGUIInfoHelper::GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
  if (window)
  {
    CGUIMediaWindow* mediaWindow = static_cast<CGUIMediaWindow*>(window);

    switch (info.m_info)
    {
      ///////////////////////////////////////////////////////////////////////////////////////////////
      // CONTAINER_* (contextWindow is a media window)
      ///////////////////////////////////////////////////////////////////////////////////////////////
      case CONTAINER_FOLDERPATH:
      case CONTAINER_FOLDERNAME:
      {
        if (info.m_info == CONTAINER_FOLDERNAME)
          value = mediaWindow->CurrentDirectory().GetLabel();
        else
          value = CURL(mediaWindow->CurrentDirectory().GetPath()).GetWithoutUserDetails();
        return true;
      }
      case CONTAINER_PLUGINNAME:
      {
        const CURL url(mediaWindow->CurrentDirectory().GetPath());
        if (url.IsProtocol("plugin"))
        {
          value = URIUtils::GetFileName(url.GetHostName());
          return true;
        }
        break;
      }
      case CONTAINER_VIEWCOUNT:
      case CONTAINER_VIEWMODE:
      {
        const CGUIControl *control = mediaWindow->GetControl(mediaWindow->GetViewContainerID());
        if (control && control->IsContainer())
        {
          if (info.m_info == CONTAINER_VIEWMODE)
          {
            value = static_cast<const IGUIContainer*>(control)->GetLabel();
            return true;
          }
          else if (info.m_info == CONTAINER_VIEWCOUNT)
          {
            value = StringUtils::Format("%i", mediaWindow->GetViewCount());
            return true;
          }
        }
        break;
      }
      case CONTAINER_SORT_METHOD:
      case CONTAINER_SORT_ORDER:
      {
        const CGUIViewState *viewState = mediaWindow->GetViewState();
        if (viewState)
        {
          if (info.m_info == CONTAINER_SORT_METHOD)
          {
            value = g_localizeStrings.Get(viewState->GetSortMethodLabel());
            return true;
          }
          else if (info.m_info == CONTAINER_SORT_ORDER)
          {
            value = g_localizeStrings.Get(viewState->GetSortOrderLabel());
            return true;
          }
        }
        break;
      }
      case CONTAINER_PROPERTY:
      {
        value = mediaWindow->CurrentDirectory().GetProperty(info.GetData3()).asString();
        return true;
      }
      case CONTAINER_ART:
      {
        value = mediaWindow->CurrentDirectory().GetArt(info.GetData3());
        return true;
      }
      case CONTAINER_CONTENT:
      {
        value = mediaWindow->CurrentDirectory().GetContent();
        return true;
      }
      case CONTAINER_SHOWPLOT:
      {
        value = mediaWindow->CurrentDirectory().GetProperty("showplot").asString();
        return true;
      }
      case CONTAINER_SHOWTITLE:
      {
        value = mediaWindow->CurrentDirectory().GetProperty("showtitle").asString();
        return true;
      }
      case CONTAINER_PLUGINCATEGORY:
      {
        value = mediaWindow->CurrentDirectory().GetProperty("plugincategory").asString();
        return true;
      }
      case CONTAINER_TOTALTIME:
      case CONTAINER_TOTALWATCHED:
      case CONTAINER_TOTALUNWATCHED:
      {
        const CFileItemList& items = mediaWindow->CurrentDirectory();
        int count = 0;
        for (const auto& item : items)
        {
          // Iterate through container and count watched, unwatched and total duration.
          if (info.m_info == CONTAINER_TOTALWATCHED && item->HasVideoInfoTag() && item->GetVideoInfoTag()->GetPlayCount() > 0)
            count += 1;
          else if (info.m_info == CONTAINER_TOTALUNWATCHED && item->HasVideoInfoTag() && item->GetVideoInfoTag()->GetPlayCount() == 0)
            count += 1;
          else if (info.m_info == CONTAINER_TOTALTIME && item->HasMusicInfoTag())
            count += item->GetMusicInfoTag()->GetDuration();
          else if (info.m_info == CONTAINER_TOTALTIME && item->HasVideoInfoTag())
            count += item->GetVideoInfoTag()->m_streamDetails.GetVideoDuration();
        }
        if (info.m_info == CONTAINER_TOTALTIME && count > 0)
        {
          value = StringUtils::SecondsToTimeString(count);
          return true;
        }
        else if (info.m_info == CONTAINER_TOTALWATCHED || info.m_info == CONTAINER_TOTALUNWATCHED)
        {
          value = StringUtils::Format("%i", count);
          return true;
        }
        break;
      }
    }
  }

  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // CONTAINER_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case CONTAINER_NUM_PAGES:
    case CONTAINER_CURRENT_PAGE:
    case CONTAINER_NUM_ITEMS:
    case CONTAINER_POSITION:
    case CONTAINER_ROW:
    case CONTAINER_COLUMN:
    case CONTAINER_CURRENT_ITEM:
    case CONTAINER_NUM_ALL_ITEMS:
    case CONTAINER_NUM_NONFOLDER_ITEMS:
    {
      const CGUIControl *control = nullptr;
      if (info.GetData1())
      { // container specified
        CGUIWindow *window = CGUIInfoHelper::GetWindowWithCondition(contextWindow, 0);
        if (window)
          control = window->GetControl(info.GetData1());
      }
      else
      { // no container specified - assume a mediawindow
        CGUIWindow *window = CGUIInfoHelper::GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
        if (window)
          control = window->GetControl(window->GetViewContainerID());
      }
      if (control)
      {
        if (control->IsContainer())
          value = static_cast<const IGUIContainer*>(control)->GetLabel(info.m_info);
        else if (control->GetControlType() == CGUIControl::GUICONTROL_GROUPLIST)
          value = static_cast<const CGUIControlGroupList*>(control)->GetLabel(info.m_info);
        else if (control->GetControlType() == CGUIControl::GUICONTROL_TEXTBOX)
          value = static_cast<const CGUITextBox*>(control)->GetLabel(info.m_info);
        return true;
      }
      break;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // CONTROL_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case CONTROL_GET_LABEL:
    {
      CGUIWindow *window = CGUIInfoHelper::GetWindowWithCondition(contextWindow, 0);
      if (window)
      {
        const CGUIControl *control = window->GetControl(info.GetData1());
        if (control)
        {
          int data2 = info.GetData2();
          if (data2)
            value = control->GetDescriptionByIndex(data2);
          else
            value = control->GetDescription();
          return true;
        }
      }
      break;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // WINDOW_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case WINDOW_PROPERTY:
    {
      CGUIWindow *window = nullptr;
      if (info.GetData1())
      { // window specified
        window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(info.GetData1());
      }
      else
      { // no window specified - assume active
        window = CGUIInfoHelper::GetWindowWithCondition(contextWindow, 0);
      }
      if (window)
      {
        value = window->GetProperty(info.GetData3()).asString();
        return true;
      }
      break;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // SYSTEM_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case SYSTEM_CURRENT_WINDOW:
      value = g_localizeStrings.Get(CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog());
      return true;
    case SYSTEM_STARTUP_WINDOW:
      value = StringUtils::Format("%i", CServiceBroker::GetSettings().GetInt(CSettings::SETTING_LOOKANDFEEL_STARTUPWINDOW));
      return true;
    case SYSTEM_CURRENT_CONTROL:
    case SYSTEM_CURRENT_CONTROL_ID:
    {
      CGUIWindow *window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog());
      if (window)
      {
        CGUIControl *control = window->GetFocusedControl();
        if (control)
        {
          if (info.m_info == SYSTEM_CURRENT_CONTROL_ID)
            value = StringUtils::Format("%i", control->GetID());
          else if (info.m_info == SYSTEM_CURRENT_CONTROL)
            value = control->GetDescription();
          return true;
        }
      }
      break;
    }
    case SYSTEM_PROGRESS_BAR:
    {
      CGUIDialogProgress *bar = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);
      if (bar && bar->IsDialogRunning())
        value = StringUtils::Format("%i", bar->GetPercentage());
      return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // FANART_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case FANART_COLOR1:
    {
      CGUIWindow *window = CGUIInfoHelper::GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        value = static_cast<CGUIMediaWindow*>(window)->CurrentDirectory().GetProperty("fanart_color1").asString();
        return true;
      }
      break;
    }
    case FANART_COLOR2:
    {
      CGUIWindow *window = CGUIInfoHelper::GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        value = static_cast<CGUIMediaWindow*>(window)->CurrentDirectory().GetProperty("fanart_color2").asString();
        return true;
      }
      break;
    }
    case FANART_COLOR3:
    {
      CGUIWindow *window = CGUIInfoHelper::GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        value = static_cast<CGUIMediaWindow*>(window)->CurrentDirectory().GetProperty("fanart_color3").asString();
        return true;
      }
      break;
    }
    case FANART_IMAGE:
    {
      CGUIWindow *window = CGUIInfoHelper::GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        value = static_cast<CGUIMediaWindow*>(window)->CurrentDirectory().GetArt("fanart");
        return true;
      }
      break;
    }
  }

  return false;
}

bool CGUIControlsGUIInfo::GetInt(int& value, const CGUIListItem *gitem, int contextWindow, const GUIInfo &info) const
{
  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // SYSTEM_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case SYSTEM_PROGRESS_BAR:
    {
      CGUIDialogProgress *bar = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);
      if (bar && bar->IsDialogRunning())
        value = bar->GetPercentage();
      return true;
    }
  }

  return false;
}

bool CGUIControlsGUIInfo::GetBool(bool& value, const CGUIListItem *gitem, int contextWindow, const GUIInfo &info) const
{
  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // CONTAINER_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case CONTAINER_HASFILES:
    case CONTAINER_HASFOLDERS:
    {
      CGUIWindow *window = CGUIInfoHelper::GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        const CFileItemList& items = static_cast<CGUIMediaWindow*>(window)->CurrentDirectory();
        for (const auto& item : items)
        {
          if ((!item->m_bIsFolder && info.m_info == CONTAINER_HASFILES) ||
              (item->m_bIsFolder && !item->IsParentFolder() && info.m_info == CONTAINER_HASFOLDERS))
          {
            value = true;
            return true;
          }
        }
      }
      break;
    }
    case CONTAINER_STACKED:
    {
      CGUIWindow *window = CGUIInfoHelper::GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        value = static_cast<CGUIMediaWindow*>(window)->CurrentDirectory().GetProperty("isstacked").asBoolean();
        return true;
      }
      break;
    }
    case CONTAINER_HAS_THUMB:
    {
      CGUIWindow *window = CGUIInfoHelper::GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        value = static_cast<CGUIMediaWindow*>(window)->CurrentDirectory().HasArt("thumb");
        return true;
      }
      break;
    }
    case CONTAINER_CAN_FILTER:
    {
      CGUIWindow *window = CGUIInfoHelper::GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        value = static_cast<CGUIMediaWindow*>(window)->CanFilterAdvanced();
        return true;
      }
      break;
    }
    case CONTAINER_CAN_FILTERADVANCED:
    {
      CGUIWindow *window = CGUIInfoHelper::GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        value = static_cast<CGUIMediaWindow*>(window)->CanFilterAdvanced();
        return true;
      }
      break;
    }
    case CONTAINER_FILTERED:
    {
      CGUIWindow *window = CGUIInfoHelper::GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        value = static_cast<CGUIMediaWindow*>(window)->IsFiltered();
        return true;
      }
      break;
    }
    case CONTAINER_SORT_METHOD:
    {
      CGUIWindow *window = CGUIInfoHelper::GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        const CGUIViewState *viewState = static_cast<CGUIMediaWindow*>(window)->GetViewState();
        if (viewState)
        {
          value = (static_cast<unsigned int>(viewState->GetSortMethod().sortBy) == info.GetData1());
          return true;
        }
      }
      break;
    }
    case CONTAINER_SORT_DIRECTION:
    {
      CGUIWindow *window = CGUIInfoHelper::GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        const CGUIViewState *viewState = static_cast<CGUIMediaWindow*>(window)->GetViewState();
        if (viewState)
        {
          value = (static_cast<unsigned int>(viewState->GetSortOrder()) == info.GetData1());
          return true;
        }
      }
      break;
    }
    case CONTAINER_CONTENT:
    {
      std::string content;
      CGUIWindow *window = CGUIInfoHelper::GetWindowWithCondition(contextWindow, 0);
      if (window)
      {
        if (window->GetID() == WINDOW_DIALOG_MUSIC_INFO)
          content = static_cast<CGUIDialogMusicInfo*>(window)->GetContent();
        else if (window->GetID() == WINDOW_DIALOG_SONG_INFO)
          content = static_cast<CGUIDialogSongInfo*>(window)->GetContent();
        else if (window->GetID() == WINDOW_DIALOG_VIDEO_INFO)
          content = static_cast<CGUIDialogVideoInfo*>(window)->CurrentDirectory().GetContent();
      }
      if (content.empty())
      {
        window = CGUIInfoHelper::GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
        if (window)
          content = static_cast<CGUIMediaWindow*>(window)->CurrentDirectory().GetContent();
      }
      value = StringUtils::EqualsNoCase(info.GetData3(), content);
      return true;
    }
    case CONTAINER_ROW:
    case CONTAINER_COLUMN:
    case CONTAINER_POSITION:
    case CONTAINER_HAS_NEXT:
    case CONTAINER_HAS_PREVIOUS:
    case CONTAINER_SCROLLING:
    case CONTAINER_SUBITEM:
    case CONTAINER_ISUPDATING:
    case CONTAINER_HAS_PARENT_ITEM:
    {
      if (info.GetData1())
      {
        CGUIWindow *window = CGUIInfoHelper::GetWindowWithCondition(contextWindow, 0);
        if (window)
        {
          const CGUIControl *control = window->GetControl(info.GetData1());
          if (control)
          {
            value = control->GetCondition(info.m_info, info.GetData2());
            return true;
          }
        }
      }
      else
      {
        const CGUIControl *activeContainer = CGUIInfoHelper::GetActiveContainer(0, contextWindow);
        if (activeContainer)
        {
          value = activeContainer->GetCondition(info.m_info, info.GetData2());
          return true;
        }
      }
      break;
    }
    case CONTAINER_HAS_FOCUS:
    { // grab our container
      CGUIWindow *window = CGUIInfoHelper::GetWindowWithCondition(contextWindow, 0);
      if (window)
      {
        const CGUIControl *control = window->GetControl(info.GetData1());
        if (control && control->IsContainer())
        {
          const CFileItemPtr item = std::static_pointer_cast<CFileItem>(static_cast<const IGUIContainer*>(control)->GetListItem(0));
          if (item && item->m_iprogramCount == info.GetData2())  // programcount used to store item id
          {
            value = true;
            return true;
          }
        }
      }
      break;
    }
    case CONTAINER_SCROLL_PREVIOUS:
    case CONTAINER_MOVE_PREVIOUS:
    case CONTAINER_MOVE_NEXT:
    case CONTAINER_SCROLL_NEXT:
    {
      int containerId = -1;
      if (info.GetData1())
      {
        containerId = info.GetData1();
      }
      else
      {
        // no parameters, so we assume it's just requested for a media window.  It therefore
        // can only happen if the list has focus.
        CGUIWindow *pWindow = CGUIInfoHelper::GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
        if (pWindow)
          containerId = pWindow->GetViewContainerID();
      }
      if (containerId != -1)
      {
        const std::map<int,int>::const_iterator it = m_containerMoves.find(containerId);
        if (it != m_containerMoves.end())
        {
          if (info.m_info == CONTAINER_SCROLL_PREVIOUS)
            value = it->second <= -2;
          else if (info.m_info == CONTAINER_MOVE_PREVIOUS)
            value = it->second <= -1;
          else if (info.m_info == CONTAINER_MOVE_NEXT)
            value = it->second >= 1;
          else if (info.m_info == CONTAINER_SCROLL_NEXT)
            value = it->second >= 2;
          return true;
        }
      }
      break;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // CONTROL_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case CONTROL_IS_VISIBLE:
    {
      CGUIWindow *window = CGUIInfoHelper::GetWindowWithCondition(contextWindow, 0);
      if (window)
      {
        // Note: This'll only work for unique id's
        const CGUIControl *control = window->GetControl(info.GetData1());
        if (control)
        {
          value = control->IsVisible();
          return true;
        }
      }
      break;
    }
    case CONTROL_IS_ENABLED:
    {
      CGUIWindow *window = CGUIInfoHelper::GetWindowWithCondition(contextWindow, 0);
      if (window)
      {
        // Note: This'll only work for unique id's
        const CGUIControl *control = window->GetControl(info.GetData1());
        if (control)
        {
          value = !control->IsDisabled();
          return true;
        }
      }
      break;
    }
    case CONTROL_HAS_FOCUS:
    {
      CGUIWindow *window = CGUIInfoHelper::GetWindowWithCondition(contextWindow, 0);
      if (window)
      {
        value = (window->GetFocusedControlID() == static_cast<int>(info.GetData1()));
        return true;
      }
      break;
    }
    case CONTROL_GROUP_HAS_FOCUS:
    {
      CGUIWindow *window = CGUIInfoHelper::GetWindowWithCondition(contextWindow, 0);
      if (window)
      {
        value = window->ControlGroupHasFocus(info.GetData1(), info.GetData2());
        return true;
      }
      break;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // WINDOW_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case WINDOW_IS_MEDIA:
    { // note: This doesn't return true for dialogs (content, favourites, login, videoinfo)
      CGUIWindowManager& windowMgr = CServiceBroker::GetGUI()->GetWindowManager();
      CGUIWindow *window = windowMgr.GetWindow(windowMgr.GetActiveWindow());
      if (window)
      {
        value = window->IsMediaWindow();
        return true;
      }
      break;
    }
    case WINDOW_IS:
    {
      if (info.GetData1())
      {
        CGUIWindowManager& windowMgr = CServiceBroker::GetGUI()->GetWindowManager();
        CGUIWindow *window = windowMgr.GetWindow(contextWindow);
        if (!window)
        {
          // try topmost dialog
          window = windowMgr.GetWindow(windowMgr.GetTopmostModalDialog());
          if (!window)
          {
            // try active window
            window = windowMgr.GetWindow(windowMgr.GetActiveWindow());
          }
        }
        if (window)
        {
          value = (window && window->GetID() == static_cast<int>(info.GetData1()));
          return true;
        }
      }
      break;
    }
    case WINDOW_IS_VISIBLE:
    {
      if (info.GetData1())
        value = CServiceBroker::GetGUI()->GetWindowManager().IsWindowVisible(info.GetData1());
      else
        value = CServiceBroker::GetGUI()->GetWindowManager().IsWindowVisible(info.GetData3());
      return true;
    }
    case WINDOW_IS_ACTIVE:
    {
      if (info.GetData1())
        value = CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(info.GetData1());
      else
        value = CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(info.GetData3());
      return true;
    }
    case WINDOW_IS_DIALOG_TOPMOST:
    {
      if (info.GetData1())
        value = CServiceBroker::GetGUI()->GetWindowManager().IsDialogTopmost(info.GetData1());
      else
        value = CServiceBroker::GetGUI()->GetWindowManager().IsDialogTopmost(info.GetData3());
      return true;
    }
    case WINDOW_IS_MODAL_DIALOG_TOPMOST:
    {
      if (info.GetData1())
        value = CServiceBroker::GetGUI()->GetWindowManager().IsModalDialogTopmost(info.GetData1());
      else
        value = CServiceBroker::GetGUI()->GetWindowManager().IsModalDialogTopmost(info.GetData3());
      return true;
    }
    case WINDOW_NEXT:
    {
      if (info.GetData1())
      {
        value = (static_cast<int>(info.GetData1()) == m_nextWindowID);
        return true;
      }
      else
      {
        CGUIWindow *window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(m_nextWindowID);
        if (window && StringUtils::EqualsNoCase(URIUtils::GetFileName(window->GetProperty("xmlfile").asString()), info.GetData3()))
        {
          value = true;
          return true;
        }
      }
      break;
    }
    case WINDOW_PREVIOUS:
    {
      if (info.GetData1())
      {
        value = (static_cast<int>(info.GetData1()) == m_prevWindowID);
        return true;
      }
      else
      {
        CGUIWindow *window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(m_prevWindowID);
        if (window && StringUtils::EqualsNoCase(URIUtils::GetFileName(window->GetProperty("xmlfile").asString()), info.GetData3()))
        {
          value = true;
          return true;
        }
      }
      break;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // SYSTEM_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case SYSTEM_HAS_ACTIVE_MODAL_DIALOG:
      value = CServiceBroker::GetGUI()->GetWindowManager().HasModalDialog();
      return true;
    case SYSTEM_HAS_VISIBLE_MODAL_DIALOG:
      value = CServiceBroker::GetGUI()->GetWindowManager().HasVisibleModalDialog();
      return true;
    case SYSTEM_HAS_INPUT_HIDDEN:
    {
      CGUIDialogNumeric *pNumeric = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogNumeric>(WINDOW_DIALOG_NUMERIC);
      CGUIDialogKeyboardGeneric *pKeyboard = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogKeyboardGeneric>(WINDOW_DIALOG_KEYBOARD);

      if (pNumeric && pNumeric->IsActive())
        value = pNumeric->IsInputHidden();
      else if (pKeyboard && pKeyboard->IsActive())
        value = pKeyboard->IsInputHidden();
      return true;
    }
  }

  return false;
}
