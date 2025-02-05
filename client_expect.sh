#!/usr/bin/expect -f

set timeout 10
set username "user$env(ID)"
set password "password$env(ID)"

# Spawn the client program
spawn ./client_grp

# Wait for username prompt and send the username
expect "Enter username:"
send "$username\r"

# Wait for password prompt and send the password
expect "Enter password:"
send "$password\r"

# Wait for any prompt (e.g., main menu or chat prompt)
# expect ">"   # Modify this if needed based on actual prompt
# expect ">"
# Send the message to 'alice'
send "/msg alice hi\r"

# Keep sending 50 messages to the server (simulating a conversation or repeated messages)
for {set i 1} {$i <= 50} {incr i} {
    # expect ">"   # Wait for server prompt (modify this if needed)
    send "/msg alice hi $i\r"
    sleep 0.1
}

send "/exit\r"

# Keep the session alive and interactive if needed
interact
