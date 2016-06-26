#!/usr/bin/env python3
import socket
import select
import threading
SPLITTER_LIST=[]
Resend_list=[]
TO_FIRST_SPLITTER=[]
TO_OTHER_SPLITTERS=[]
class Tracker():
    def __init__(self):
        self.sock=socket.socket(socket.AF_INET,socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind(('',8000))
        self.sock.listen(5)
		#Tracker socket listens on port 8000
		
    """def listen(self):
        while 1:
        #Efficiently wait for sockets to be read or written to.
            if len(Resend_list) != 0:
                try:
                    Resend_list[0].send("First Splitter")
                except:
                    pass
                else:
                    Resend_list.clear()
            read,write,error=select.select(SPLITTER_LIST,SPLITTER_LIST,[],0)
            for sock in read:
                if sock == self.sock:
                    (sockfd,addr)=self.sock.accept()
                    SPLITTER_LIST.append(sockfd)
                    if len(SPLITTER_LIST)==1:
                        try:
                            sockfd.send("First Splitter")
                        #The first splitter that is appended to SPLITTER_LIST  needs to know that it is the first splitter
                        except:
                            Resend_list.append(sockfd)
                            #If the message could not be sent, send again in the next iteration
                else:
                    if sock == SPLITTER_LIST[0]:
                        TO_OTHER_SPLITTERS.append(sock.recv(1024))
                    else:
                        A header is attached to the data being sent to the underlying splitter
                        so that when we receive the response from the splitter we know who to send it to
                        TO_FIRST_SPLITTER.append(sock.recv(1024)+"!@#$!@#$"+str(sock.getpeername()))
                        #Not sure how to include the header. Suggestions welcome
                        
                
                
                
            
            for sock in write:
                if sock == SPLITTER_LIST[0]:
                    for items in TO_FIRST_SPLITTER:
                        try:
                            sock.send(items)
                        except:
                            pass
                        else:
                            TO_FIRST_SPLITTER.remove(items)
                
                else:
                    for items in TO_OTHER_SPLITTERS:
                        s=items.split("!@#$!@#$")
                        if s[1] == str(sock.getpeername()):
                            sock.send(s[0])"""
                        
                        
                        
    def listen(self):
        while True:
        #Efficiently wait for sockets to be read or written to.
            if len(Resend_list) != 0:
                try:
                    Resend_list[0].send("First Splitter")
                except Exception as e:
                    print(str(e))
                    pass
                else:
                    Resend_list.clear()
            (sockfd,addr)=self.sock.accept()
            SPLITTER_LIST.append(sockfd)
            if len(SPLITTER_LIST)==1:
                try:
                    sockfd.send(b"First Splitter")
                    #Since this is a multi-threaded environment, we can identify the first splitter through the splitter_id  variable passed on to the sync function
                    #However, the splitter should know that it is the first splitter
                except Exception as e:
                    print("".join(str(e)))
                    Resend_list.append(sockfd)
                    #If the message could not be sent, send again in the next iteration
                    #Resend_list also can be removed
            try:
                splitter_id=len(SPLITTER_LIST)-1
                threading.Thread(target=self.sync,args=(splitter_id,)).start()
            except Exception as e:
                print("Thread could not be started "+str(e))

    
    def sync(self,splitter_id):
        if splitter_id==0:
            SPLITTER_LIST[0].settimeout(5)
            #sock.recv times out after 5 seconds if there is no data to be received
            while True:
                for items in TO_FIRST_SPLITTER:
                    try:
                        SPLITTER_LIST[0].send(items)
                    except:
                        pass
                    else:
                        TO_FIRST_SPLITTER.remove(items)
                    
                try:
                    offset=SPLITTER_LIST[0].recv(1024)
                    if offset=='':
                        return
                    TO_OTHER_SPLITTERS.append(offset)
                except:
                    pass
            
        else:
            chunk_hash=SPLITTER_LIST[splitter_id].recv(1024)
            if chunk_hash == '':
                return
            TO_FIRST_SPLITTER.append(chunk_hash+"!@#$!@#$".encode('ASCII')+str(SPLITTER_LIST[splitter_id].getpeername()).encode('ASCII'))
            while len(TO_OTHER_SPLITTERS)==0:
                pass
                #Wait till some data is entered into the above list
            for items in TO_OTHER_SPLITTERS: 
                item=items.decode('ASCII')
                s=item.split("!@#$!@#$")
                if s[1] == str(SPLITTER_LIST[splitter_id].getpeername()):
                    try:
                        SPLITTER_LIST[splitter_id].send(s[0].encode('ASCII'))
                    except:
                        pass
                    else:
                        TO_OTHER_SPLITTERS.remove(items)
                    print("Thread exiting")
                    return 
            
            
        


                        
if __name__ == "__main__":
    track=Tracker()
    track.listen()
    
                    
                        
                
                
                
                
                
