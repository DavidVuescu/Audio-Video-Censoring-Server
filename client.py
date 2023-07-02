import socket
import os
import sys

print("--> USER CLIENT <--")

# If the user has not provided the right number of arguments, print usage instructions and return
if len(sys.argv) != 3:
    print("\nUsage:\npython client.py <Server IPv4 address> <port>\n")
    sys.exit(1)

# Server IP address and port
SERVER_IP = sys.argv[1]
SERVER_PORT = int(sys.argv[2])

# Create a TCP socket for communicating with the server
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

try:
    # Connect to the server
    client_socket.connect((SERVER_IP, SERVER_PORT))

    # Receive the welcome message from the server
    response = client_socket.recv(1024).decode()
    print(response)

    #if response != "WELCOME":
        # Server rejected the connection
     #   print("\nERROR! ---> CONNECTION REJECTED!\n")
      #  client_socket.close()
       # sys.exit(1)

    print("\n!!! CONNECTION ACCEPTED !!!\n")

    while True:
        print("\n---- MENU ----")
        print("  1. Upload video to server for censoring")
        print("  2. Exit session.")
        print("> ")

        # Read the user's option
        try:
            op = int(input())
        except ValueError:
            print("\nE: Please provide a valid option from the list and try again!")
            continue

        if op == 1:
            # Prompt the user for the video file path
            path = input("\nPlease type the path to the video file: ")

            if not os.path.isfile(path):
                print("\nE: File not found.")
                continue

            # Get the video file size
            file_size = os.path.getsize(path)

            # Send the response to the server in the format "EXTENSION SIZE"
            response = f"{os.path.splitext(path)[1]} {file_size}"
            client_socket.send(response.encode())

            # Receive the response from the server
            response = client_socket.recv(1024).decode()
            
            
           #f response is "SEND_FILE":
            	
            	#print("\nE: Unexpected change in communication protocols.")
                #break

            # Upload the video file to the server
            with open(path, "rb") as file:
                while True:
                    data = file.read(4096)
                    if not data:
                        break
                    client_socket.sendall(data)

            print("\nI: Processing...")
            print("   Waiting for response from the server...")

            # Receive the response from the server
            response = client_socket.recv(1024).decode()
            
            
	    

            if response.startswith("SUCCESS"):
                #video_size = int(response.split()[1])
                video_size_str = response.split()[1].rstrip('\x00')
                try:
                	video_size = int(video_size_str)
                except ValueError:
                    print("Invalid video size:", video_size_str)
                    video_size = 0
		
                # Tell the server to start sending the video file
                client_socket.send("SEND_VIDEO".encode())			

                # Prompt the user for the path to save the downloaded video
                while True:
                    download_path = input("\nPlease provide the path to download the video: ")
                    if os.path.isfile(download_path):
                        print("\nE: File already exists. Please provide a different path.")
                        continue
                    break

                # Download the video file from the server
                with open(download_path, "wb") as file:
                    received_bytes = 0
                    while received_bytes < video_size:
                        data = client_socket.recv(4096)
                        file.write(data)
                        received_bytes += len(data)

                print(f"\nI: Success! The video file has been downloaded to: '{download_path}'")
            else:
                print("\nE: A server internal error occurred while processing the video :(")
                print("   Please try again later with different parameters...")

        elif op == 2:
            print("\nI: Ending session...")
            client_socket.send("EXIT".encode())
            break

        else:
            print("\nE: Please provide a valid option from the list and try again!")

except ConnectionRefusedError:
    print("\nERROR! Cannot connect to server!")
except KeyboardInterrupt:
    print("\nClient terminated by user.")
finally:
    # Close the socket
    client_socket.close()
