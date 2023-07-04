import socket
import os
from pathlib import Path

def main(server_ip, port):
    print("--> USER CLIENT <--")
    
    server_addr = (server_ip, int(port))

    # Create a socket object 
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM) 

    # Connection to server
    try:
        sock.connect(server_addr)
    except Exception as e:
        print("\nERROR! Cannot connect to server!\nPlease try again later!\n")
        return 3

    response = sock.recv(1024).decode()
    #if response != "WELCOME":
     #   print("\nERROR! ---> CONNECTION REJECTED!\n")
      #  sock.close()
       # return 6

    print("\n!!! CONNECTION ACCEPTED !!! \n\n")

    while True:
        print("\n---- MENU ----")
        print("  1. Upload video to server for video censoring")
        print("  2. Upload video to server for audio censoring")
        print("  3. Upload video to server for full censoring")
        print("  4. Exit session. ")
        op = int(input("> "))
        
        if op not in [1, 2, 3, 4]:
            print("\nE: Please provide correct option from the list and try again!\n")
            continue
        
        if op in [1, 2, 3]:
            sock.send(str(op).encode())

            path = input("\nPlease type path to video file: ")

            if not os.path.exists(path):
                print("\nERROR! File not found.")
                continue

            print(f"\nI: Preparing to upload file: '{path}' of size: {os.path.getsize(path)} bytes.")
            response = f"{Path(path).suffix} {os.path.getsize(path)}"
            
            # Send file information
            sock.sendall(response.encode())

            response = sock.recv(1024).decode()
            #if response != "SEND_FILE":
             #   print("\nE: Unexpected change in communication protocols.\nTerminating...")
              #  break

            with open(path, 'rb') as file:
                while (data := file.read(4096)):
                    sock.sendall(data)
            print("\nI: Processing...\nWaiting for response from the server...")

            response = sock.recv(1024).decode()
            
            video_size_str = response.split()[1].rstrip('\x00')
            try:
            	video_size = int(video_size_str)
            except ValueError:
            	print("Invalid video size:", video_size_str)
            	video_size = 0
		
            
            if "SUCCESS" in response:
                print("\nI: The server successfully created the video file!")

                # Notify the server that the client is ready to receive the file
                sock.send("SEND_VIDEO".encode())
                
                while True:
                    path = input("\nPlease provide the path to download the video: ")
                    if os.path.isfile(path):
                        print("\nE: File already exists. Please provide a different path.")
                        continue
                    break

                with open(path, "wb") as file:
                    received_bytes = 0
                    while received_bytes < video_size:
                        data = sock.recv(4096)
                        file.write(data)
                        received_bytes += len(data)
                        
                        
                        

                print(f"\nI: Success! The video file has been downloaded to: '{path}'")
            else:
                print("\nE: A server internal error occurred while processing the video :(\nPlease try again later, with different parameters...")
                continue

        elif op == 4:
            print("\nI: Ending session...")
            sock.sendall("EXIT".encode())
            break
    sock.close()

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 3:
        print(f"\nUsage:\n{sys.argv[0]} <Server IPv4 address> <port>")
        sys.exit(1)

    main(sys.argv[1], sys.argv[2])


