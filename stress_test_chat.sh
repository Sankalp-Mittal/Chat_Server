#!/bin/bash

# Ask user for the number of clients
read -p "Enter the number of clients to simulate: " NUM_CLIENTS

# Check if input is a valid number
if ! [[ "$NUM_CLIENTS" =~ ^[0-9]+$ ]]; then
    echo "Error: Please enter a valid number."
    exit 1
fi

# Run multiple clients, each in a new terminal window
for ((i = 1; i <= NUM_CLIENTS; i++)); do
    # Open a new terminal, run the expect script, and close the terminal when done using exec bash and exit
    gnome-terminal -- bash -c "ID=$i expect ./client_expect.sh; exec bash -c 'exit'" &
done

wait
echo "Stress test completed with $NUM_CLIENTS clients!"
