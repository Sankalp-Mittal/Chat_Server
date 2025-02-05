#!/bin/bash

FILE="users.txt"  # File to store usernames and passwords

# Ensure the file exists
touch "$FILE"

# Add 100 users
for i in $(seq 1 100); do
    username="user$i"
    password="password$i"

    # Append user:password pair to the file
    echo "$username: $password" >> "$FILE"
done

echo "100 temporary users added to $FILE!"
