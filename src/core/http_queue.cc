// rTorrent - BitTorrent client
// Copyright (C) 2005-2011, Jari Sundell
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

#include <memory>
#include <sstream>
#include <sigc++/adaptors/bind.h>
#include <sigc++/adaptors/hide.h>
#include <torrent/http.h>

#include "rak/functional.h"
#include "http_queue.h"
#include "curl_get.h"

namespace std { using namespace tr1; }

namespace core {

HttpQueue::iterator
HttpQueue::insert(const std::string& url, std::iostream* s) {
  std::auto_ptr<CurlGet> h(m_slotFactory());
  
  h->set_url(url);
  h->set_stream(s);
  h->set_timeout(5 * 60);

  iterator itr = Base::insert(end(), h.get());

  h->signal_done().push_back(std::bind(&HttpQueue::erase, this, itr));
  h->signal_failed().push_back(std::bind(&HttpQueue::erase, this, itr));

  (*itr)->start();

  h.release();
  m_signalInsert.emit(*itr);

  return itr;
}

void
HttpQueue::erase(iterator itr) {
  m_signalErase.emit(*itr);

  delete *itr;
  Base::erase(itr);
}

void
HttpQueue::clear() {
  while (!empty())
    erase(begin());

  Base::clear();
}

}
