import socket
import sys

def hex_to_binary(hex_string):
    """Convert hex string to binary data."""
    return bytes.fromhex(hex_string)

def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <server IP> <port>")
        return 1

    server_ip = sys.argv[1]
    port = int(sys.argv[2])

    # Create a socket object
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        # Connect to the server
        try:
            sock.connect((server_ip, port))
        except socket.error as e:
            print(f"Failed to connect to {server_ip}:{port}")
            print(f"Error: {e}")
            return 1

        print("Connected to the server. You can now send data.")
        print("Type and enter hex data to send (Ctrl-D to quit):")

        # Continue reading lines from stdin until EOF (Ctrl-D)
        try:
            while True:
                # Read line from stdin
                input_line = input()
                # Convert hex to binary
                binary_data = hex_to_binary(input_line)
                # Send binary data to the server
                sock.sendall(binary_data)
        except EOFError:
            # End of file (Ctrl-D) encountered, exit gracefully
            pass

        print("\nDisconnected from the server.")
        return 0

if __name__ == '__main__':
    sys.exit(main())