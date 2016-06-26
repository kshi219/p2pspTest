//
//  peer_core.h
//  P2PSP
//
//  This code is distributed under the GNU General Public License (see
//  THE_GENERAL_GNU_PUBLIC_LICENSE.txt for extending this information).
//  Copyright (C) 2016, the P2PSP team.
//  http://www.p2psp.org
//

#include <boost/format.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include "../lib/p2psp/src/core/common.h"
#include "../lib/p2psp/src/core/monitor_dbs.h"
#include "../lib/p2psp/src/core/monitor_lrs.h"
#include "../lib/p2psp/src/core/peer_dbs.h"
#include "../lib/p2psp/src/core/peer_ims.h"
//#include "peer_strpeds.h"
//#include "peer_strpeds_malicious.h"
//#include "trusted_peer.h"
//#include "malicious_peer.h"
#include "../lib/p2psp/src/util/trace.h"

namespace p2psp {

int run(int argc, const char* argv[]) throw(boost::system::system_error);
}
