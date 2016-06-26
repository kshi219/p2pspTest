#Testing with Splitter at Public Server

## Test Cases
### Case 1: Peer and Monitor behind same NAT router 
1. ([splitter_output_1] (https://github.com/kshi219/p2pspTest/blob/master/test_ouputs/splitter_output_1) ) lines 15-16 and ([monitor_output_1](https://github.com/kshi219/p2pspTest/blob/master/test_ouputs/monitor_ouput_1)) lines 7-9: 
Monitor connects to splitter, communicates its local endpoint


2. ([splitter_output_1] (https://github.com/kshi219/p2pspTest/blob/master/test_ouputs/splitter_output_1) ) lines 875-886 and ([peer_output_1](https://github.com/kshi219/p2pspTest/blob/master/test_ouputs/peer_output_1)) lines 7-9: 
Peer connects to splitter, communicates its local endpoint, splitter compares its public endpoint to the Monitor, concludes both are behind same NAT and sends out local endpoint of monitor. 


3. ([peer_output_1] (https://github.com/kshi219/p2pspTest/blob/master/test_ouputs/peer_output_1) ) line 24 and ([monitor_output_1](https://github.com/kshi219/p2pspTest/blob/master/test_ouputs/monitor_ouput_1)) lines 31-32: 
hello message sent by peer and recieved by monitor, this hello message is recieved because both monitor and peer are on the same host


4. note that chunks sent by the splitter via UDP are not recieved

### Case 2: Peer and Monitor not behind same NAT router
1. ([splitter_output_2] (https://github.com/kshi219/p2pspTest/blob/master/test_ouputs/splitter_output_2) ) lines 15-16 and ([monitor_output_2](https://github.com/kshi219/p2pspTest/blob/master/test_ouputs/monitor_ouput_2)) lines 7-9: 
Monitor connects to splitter, communicates its local endpoint


2. ([splitter_output_2] (https://github.com/kshi219/p2pspTest/blob/master/test_ouputs/splitter_output_2) ) lines 796-807 and ([peer_output_2](https://github.com/kshi219/p2pspTest/blob/master/test_ouputs/peer_output_2)) lines 7-9: 
Peer connects to splitter, communicates its local endpoint, splitter compares its public endpoint to the Monitor, concludes they are behind different NAT routers and sends out public endpoint of monitor. 


3. ([peer_output_2] (https://github.com/kshi219/p2pspTest/blob/master/test_ouputs/peer_output_2) ) line 24 : 
hello message sent by peer but not recieved by monitor

4. note that chunks are recieved this time by only the peer because it is at the same host as the splitter

##Commands Used

Splitter: `./splitter --source_addr 150.214.150.68 --source_port 4551 --team_port 4554 --channel BBB-134.ogv`

Monitor: `./peer -- player_port 4555 --monitor --splitter_addr 150.214.150.68 --splitter_port 4554 `

Peer: `./peer --player_port 10000 --splitter_addr 150.214.150.68 --splitter_port 4554 `
