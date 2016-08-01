//
//  splitter_ems.cc
//  P2PSP
//
//  This code is distributed under the GNU General Public License (see
//  THE_GENERAL_GNU_PUBLIC_LICENSE.txt for extending this information).
//  Copyright (C) 2016, the P2PSP team.
//  http://www.p2psp.org
//
//   EMS: Endpoint Masquerading Set of Rules
//

#include "splitter_ems.h"
#include "common_nts.h"
#include "common.h"
#include "../util/trace.h"
#include <cassert>
#include <thread>
#include <random>

namespace p2psp {
  using namespace std;
  using namespace boost::asio;


  SplitterEMS::SplitterEMS() : SplitterNTS(), peer_pairs_(0, &SplitterDBS::GetHash){

    magic_flags_ = Common::kEMS;

    TRACE("Initialized EMS");
  }

  SplitterEMS::~SplitterEMS(){}





  void SplitterEMS::SendTheListOfPeers2(
          const std::shared_ptr<ip::tcp::socket> &peer_serve_socket,
          const ip::udp::endpoint& peer) {
    ip::address target_address = peer_serve_socket->remote_endpoint().address();
    // Send all peers except the monitor peers with their peer ID
    // plus all peers currently being incorporated
    uint16_t number_of_other_peers = this->peer_list_.size()
                                     - this->max_number_of_monitors_ + this->incorporating_peers_.size();
    if (CommonNTS::Contains(this->peers_, peer)) {
      // Then peer is also in this->incorporating_peers_
      // Do not send the peer endpoint to itself
      number_of_other_peers = std::max(0, number_of_other_peers - 1);
    }
    LOG("Sending the list of peers except monitors (" << number_of_other_peers
        << " peers)");
    std::ostringstream msg_str;
    CommonNTS::Write<uint16_t>(msg_str, number_of_other_peers);
    peer_serve_socket->send(buffer(msg_str.str()));
    ip::udp::endpoint peer_endpoint;
    for (std::vector<ip::udp::endpoint>::iterator peer_iter =
            this->peer_list_.begin() + this->max_number_of_monitors_;
         peer_iter != this->peer_list_.end(); ++peer_iter) {
      // Also send the port step of the existing peer, in case
      // it is behind a sequentially allocating NAT
      const PeerInfo& peer_info = this->peers_[*peer_iter];
      msg_str.str(std::string());
      msg_str << peer_info.id_;

      peer_endpoint = boost::asio::ip::udp::endpoint(peer_iter->address(), peer_iter->port());
      //send private instead of public endpoint if target peer is in same private network
      if (peer_iter->address() == target_address) {
        peer_endpoint = peer_pairs_[peer_endpoint];
        LOG("target peer at" << target_address.to_string() << " is in a private network with peer sent as("
            << peer_endpoint.address().to_string() << "," << std::to_string(peer_endpoint.port()) << ")");
        CommonNTS::Write<uint32_t>(msg_str,
                                   (uint32_t)peer_endpoint.address().to_v4().to_ulong());
        CommonNTS::Write<uint16_t>(msg_str, peer_endpoint.port());
        CommonNTS::Write<uint16_t>(msg_str, peer_info.port_step_);
      } else  {
        LOG("sent peer("
            << peer_iter->address().to_string() << "," << std::to_string(peer_info.last_source_port_) << ")");
        CommonNTS::Write<uint32_t>(msg_str,
                                   (uint32_t)peer_iter->address().to_v4().to_ulong());
        CommonNTS::Write<uint16_t>(msg_str, peer_info.last_source_port_);
        CommonNTS::Write<uint16_t>(msg_str, peer_info.port_step_);
      }

      peer_serve_socket->send(buffer(msg_str.str()));
    }
    // Send the peers currently being incorporated
    for (const auto& peer_iter : this->incorporating_peers_) {
      const std::string& peer_id = peer_iter.first;
      // Do not send the peer endpoint to itself
      if (CommonNTS::Contains(this->peers_, peer)
          && peer_id == this->peers_[peer].id_) {
        continue;
      }
      LOG("Sending peer " << peer_id << " to " << peer);
      const ip::udp::endpoint& peer = peer_iter.second.peer_;
      const PeerInfo& peer_info = this->peers_[peer];
      msg_str.str(std::string());
      msg_str << peer_id;
      if (peer.address() == target_address) {
        peer_endpoint = peer_pairs_[peer];
        LOG("target peer at" << target_address.to_string() << " is in a private network with peer sent as("
            << peer_endpoint.address().to_string() << "," << std::to_string(peer_endpoint.port()) << ")");
        CommonNTS::Write<uint32_t>(msg_str,
                                   (uint32_t)peer_endpoint.address().to_v4().to_ulong());
        CommonNTS::Write<uint16_t>(msg_str, peer_endpoint.port());
        CommonNTS::Write<uint16_t>(msg_str, peer_info.port_step_);
      } else {
        LOG("sent peer("
            << peer.address().to_string() << "," << std::to_string(peer_info.last_source_port_) << ")");
        CommonNTS::Write<uint32_t>(msg_str,
                                   (uint32_t)peer.address().to_v4().to_ulong());
        CommonNTS::Write<uint16_t>(msg_str, peer_info.last_source_port_);
        CommonNTS::Write<uint16_t>(msg_str, peer_info.port_step_);
      }
      peer_serve_socket->send(buffer(msg_str.str()));
    }
  }


  void SplitterEMS::RemovePeer(const ip::udp::endpoint& peer) {
    SplitterLRS::RemovePeer(peer);

    try {
      this->peers_.erase(peer);
      peer_pairs_.erase(peer);
    } catch (std::exception e) {
      TRACE(e.what());
      // ignore
    }
  }



  void SplitterEMS::HandleAPeerArrival(
          std::shared_ptr<boost::asio::ip::tcp::socket> serve_socket) {
    // This method implements the NAT traversal algorithms.

    boost::asio::ip::tcp::endpoint new_peer_tcp = serve_socket->remote_endpoint();
    boost::asio::ip::udp::endpoint new_peer(new_peer_tcp.address(), new_peer_tcp.port());
    LOG("Accepted connection from peer " << new_peer);
    boost::array<char, 6> buf;
    char *raw_data = buf.data();
    boost::asio::ip::address ip_addr;
    int port;

    read((*serve_socket), boost::asio::buffer(buf));
    in_addr ip_raw = *(in_addr *)(raw_data);
    ip_addr = boost::asio::ip::address::from_string(inet_ntoa(ip_raw));
    port = ntohs(*(short *)(raw_data + 4));

    boost::asio::ip::udp::endpoint peer_local_endpoint_ = boost::asio::ip::udp::endpoint(ip_addr, port);

    TRACE("peer local endpoint = (" << peer_local_endpoint_.address().to_string() << ","
          << std::to_string(peer_local_endpoint_.port()) << ")");

    SplitterEMS::peer_pairs_.emplace(boost::asio::ip::udp::endpoint(new_peer_tcp.address(),
                                                                    new_peer_tcp.port()), peer_local_endpoint_);
    this->SendConfiguration(serve_socket);
    this->SendTheListOfPeers(serve_socket);
    // Send the generated ID to peer
    std::string peer_id = this->GenerateId();
    LOG("Sending ID " << peer_id << " to peer " << new_peer);
    serve_socket->send(boost::asio::buffer(peer_id));
    std::unique_lock<std::mutex> lock(arriving_incorporating_peers_mutex_);
    if (this->peer_list_.size() < (unsigned int) this->max_number_of_monitors_) {
      // Directly incorporate the monitor peer into the team.
      // The source ports are all set to the same, as the monitor peers
      // should be publicly accessible
      this->peers_[new_peer] = PeerInfo{peer_id, .port_step_ = 0,
              .last_source_port_ = new_peer.port()};
      this->SendNewPeer(peer_id, new_peer,
                        std::vector<uint16_t>(this->max_number_of_monitors_, new_peer.port()), 0);
      this->InsertPeer(new_peer);
      serve_socket->close();
    } else {
      this->arriving_peers_[peer_id] = ArrivingPeerInfo{serve_socket,
                                                        new_peer.address(), 0,
                                                        std::vector<uint16_t>(this->max_number_of_monitors_, 0),
                                                        std::chrono::steady_clock::now()};
      // Splitter will continue with IncorporatePeer() as soon as the
      // arriving peer has sent UDP packets to splitter and monitor
    }
  }


  //mostly identical to same method in NTS, with addition EMS checks
  void SplitterEMS::SendNewPeer(const std::string& peer_id,
                                const ip::udp::endpoint& new_peer,
                                const std::vector<uint16_t>& source_ports_to_monitors, uint16_t port_step) {

    // Recreate this->extra_socket_, listening to a random port
    // TODO: is the extra_socket recreated too often?
    if (this->extra_socket_)
    {
      this->extra_socket_->close();
    }
    this->extra_socket_.reset(new ip::udp::socket(this->io_service_));
    this->extra_socket_->open(ip::udp::v4());
    try {
      this->extra_socket_->bind(ip::udp::endpoint(ip::udp::v4(), 0));
    } catch (std::exception e) {
      ERROR(e.what());
    }
    // Do not block the thread forever:
    this->extra_socket_->set_option(socket_base::linger(true, 1));
    uint16_t extra_listen_port = this->extra_socket_->local_endpoint().port();
    DEBUG("Listening to the extra port " << extra_listen_port);
    DEBUG("Sending [send hello to " << new_peer << ']');
    // The peers start port prediction at the minimum known source port,
    // counting up using their peer_number
    std::vector<uint16_t> source_ports(source_ports_to_monitors);
    source_ports.push_back(new_peer.port());
    uint16_t min_known_source_port = *std::min_element(source_ports.begin(),
                                                       source_ports.end());
    // Send packets to all peers;
    unsigned int peer_number = 0;
    for (auto peer_iter = this->peer_list_.begin();
         peer_iter != this->peer_list_.end(); ++peer_iter, ++peer_number) {
      std::ostringstream msg_str;
      msg_str << peer_id;
      if (peer_number < (unsigned int) this->max_number_of_monitors_) {
        // Send only the endpoint of the peer to the monitor,
        // as the arriving peer and the monitor already communicated
        CommonNTS::Write<uint32_t>(msg_str,
                                   (uint32_t)new_peer.address().to_v4().to_ulong());
        CommonNTS::Write<uint16_t>(msg_str,
                                   source_ports_to_monitors[peer_number]);
      } else {
        // Send all information necessary for port prediction to the
        // existing peers

        //EMS check
        ip::udp::endpoint sent_peer = new_peer;
        if (new_peer.address() == peer_iter->address())
        {
          sent_peer = peer_pairs_[new_peer];
        }

        CommonNTS::Write<uint32_t>(msg_str,
                                   (uint32_t) sent_peer.address().to_v4().to_ulong());
        CommonNTS::Write<uint16_t>(msg_str, min_known_source_port);
        CommonNTS::Write<uint16_t>(msg_str, port_step);
        // Splitter is "peer number 0", thus add 1
        CommonNTS::Write<uint16_t>(msg_str, peer_number+1);
        if (this->peers_[*peer_iter].port_step_ != 0) {
          // Send the port of this->extra_socket_ to determine the
          // currently allocated source port of the incorporated peer
          CommonNTS::Write<uint16_t>(msg_str, extra_listen_port);
        }
      }

      // Hopefully one of these packets arrives
      this->EnqueueMessage(3, std::make_pair(msg_str.str(), *peer_iter));
    }

    // Send packets to peers currently being incorporated
    for (const auto& peer_iter : this->incorporating_peers_) {
      const std::string& inc_peer_id = peer_iter.first;
      if (inc_peer_id == peer_id) {
        // Do not send the peer endpoint to the peer itself
        continue;
      }

      LOG("Sending peer " << new_peer << " to " << inc_peer_id);
      const ip::udp::endpoint& peer = peer_iter.second.peer_;

      //EMS check
      ip::udp::endpoint sent_peer = new_peer;
      if (new_peer.address() == peer.address())
      {
        sent_peer = peer_pairs_[new_peer];
      }

      std::ostringstream msg_str;
      msg_str << peer_id;
      CommonNTS::Write<uint32_t>(msg_str,
                                 (uint32_t)sent_peer.address().to_v4().to_ulong());
      CommonNTS::Write<uint16_t>(msg_str, min_known_source_port);
      CommonNTS::Write<uint16_t>(msg_str, port_step);
      // Send the length of the peer_list as peer_number
      CommonNTS::Write<uint16_t>(msg_str, this->peer_list_.size()+1);
      // Hopefully one of these packets arrives
      this->EnqueueMessage(3, std::make_pair(msg_str.str(), peer));
    }
  }

}
