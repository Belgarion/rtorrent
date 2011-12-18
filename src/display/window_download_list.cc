// rTorrent - BitTorrent client
// Copyright (C) 2005-2007, Jari Sundell
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// In addition, as a special exception, the copyright holders give
// permission to link the code of portions of this program with the
// OpenSSL library under certain conditions as described in each
// individual source file, and distribute linked combinations
// including the two.
//
// You must obey the GNU General Public License in all respects for
// all of the code used other than OpenSSL.  If you modify file(s)
// with this exception, you may extend this exception to your version
// of the file(s), but you are not obligated to do so.  If you do not
// wish to do so, delete this exception statement from your version.
// If you delete this exception statement from all source files in the
// program, then also delete it here.
//
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#include "config.h"

#include <rak/algorithm.h>

#include <torrent/rate.h>

#include "core/download.h"
#include "core/view.h"

#include "canvas.h"
#include "globals.h"
#include "utils.h"
#include "window_download_list.h"

namespace display {

WindowDownloadList::~WindowDownloadList() {
  m_connChanged.disconnect();
}

void
WindowDownloadList::set_view(core::View* l) {
  m_view = l;

  m_connChanged.disconnect();

  if (m_view != NULL)
    m_connChanged = m_view->signal_changed().connect(sigc::mem_fun(*this, &Window::mark_dirty));
}

void
WindowDownloadList::redraw() {
  /*timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);*/

  m_slotSchedule(this, (cachedTime + rak::timer::from_seconds(1)).round_seconds());

  m_canvas->erase();

  if (m_view == NULL)
    return;

  m_canvas->print(0, 0, "%s", ("[View: " + m_view->name() + "]").c_str());

  if (m_view->empty_visible() || m_canvas->width() < 5 || m_canvas->height() < 2)
    return;

  typedef std::pair<core::View::iterator, core::View::iterator> Range;

  bool minimal = false;
  if (m_view->name() == "minimal") {
	  minimal = true;
  }

  Range range;
  if (!minimal) {
	  range = rak::advance_bidirectional(m_view->begin_visible(),
											   m_view->focus() != m_view->end_visible() ? m_view->focus() : m_view->begin_visible(),
											   m_view->end_visible(),
											   m_canvas->height() / 3 - 1);
  } else {
	  range = rak::advance_bidirectional(m_view->begin_visible(),
											   m_view->focus() != m_view->end_visible() ? m_view->focus() : m_view->begin_visible(),
											   m_view->end_visible(),
											   m_canvas->height() - 2);
  }

  // Make sure we properly fill out the last lines so it looks like
  // there are more torrents, yet don't hide it if we got the last one
  // in focus.
  if (range.second != m_view->end_visible() && !minimal)
    ++range.second;

  int bg = COLOR_BLACK;
#ifdef HAVE_NCURSES_USE_DEFAULT_COLORS
  bg = -1;
#endif

  static bool hasColor = has_colors();
  static bool colorsInitialized = false;
  if (hasColor && !colorsInitialized) {
	  init_pair(1, COLOR_RED, bg);
	  init_pair(2, COLOR_GREEN, bg);
	  init_pair(3, COLOR_YELLOW, bg);
	  init_pair(4, COLOR_BLUE, bg);
	  init_pair(8, COLOR_WHITE, bg);
	  colorsInitialized = true;
  }

  int pos = 1;



  double ratio = 0;
  int col = 0;
  int statusCol = 0;
  int statusAttrib = 0;

  int width = m_canvas->width();

  if (minimal) {
	char buffer[width + 1];
	char* last = buffer + width - 2 + 1;
	print_download_minimal_header(buffer, last);
	m_canvas->print(0, pos, "%c %s", ' ', buffer);
	pos++;
  }

  while (range.first != range.second) {
    char buffer[width + 1];
    char* position;
    char* last = buffer + width - 2 + 1;


	ratio = ((*range.first)->download()->bytes_done()) ? (double)(*range.first)->info()->up_rate()->total() / (double)(*range.first)->download()->bytes_done() : 0;

	if (hasColor) {
		if (ratio >= 1.0)       { col = 2; }
		else if (ratio >= 0.5)  { col = 3; }
		else					{ col = 1; }

		if ( (*range.first)->is_seeding()) {
			statusCol = 2; //Green
			statusAttrib = A_BOLD;
		} else if ( (*range.first)->is_downloading()) {
			statusCol = 4; //Blue
			statusAttrib = A_BOLD;
		} else {
			statusCol = 1; //Red
		}
	}

	char focused = ' ';
	if (range.first == m_view->focus())
		focused = '*';

	if (!minimal) {
		position = print_download_title(buffer, last, *range.first, ratio);
		m_canvas->print(0, pos, "%c %s", focused, buffer);
		if (hasColor) {
			m_canvas->set_attr(0, pos, width, statusAttrib, statusCol);
			m_canvas->set_attr( (*range.first)->info()->name().size()+3, pos, width, 0, col);
		}
		pos++;

		position = print_download_info(buffer, last, *range.first);
		m_canvas->print(0, pos++, "%c %s", focused, buffer);

		position = print_download_status(buffer, last, *range.first);
		m_canvas->print(0, pos++, "%c %s", focused, buffer);
	} else {
		position = print_download_minimal(buffer, last, *range.first, ratio);
		m_canvas->print(0, pos, "%c %s", focused, buffer);
		if (hasColor) {
			m_canvas->set_attr(0, pos, 58, statusAttrib, statusCol);
			m_canvas->set_attr(61, pos, 5, 0, col);
		}
		pos++;
	}

    ++range.first;
  }
 /* timespec ts2;
  clock_gettime(CLOCK_REALTIME, &ts2);
  m_canvas->print(0, 0, "Time: %d    Height: %d  Width: %d   pos: %d   ", ts2.tv_nsec - ts.tv_nsec, m_canvas->height(), 
		  width, pos);*/
}

}
