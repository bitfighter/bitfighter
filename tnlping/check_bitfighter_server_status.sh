#!/bin/bash
#
# Bitfighter dedicated/master server checker
#
# Change variables in this file to suit your needs
#
# You can run this as a cron job by editing the crontab and adding a time pattern.  Example:
#
# */10 * * * * /root/bin/check_bitfighter_server_status.sh
#
# The above pattern checks the server every 10 minutes
#
# NOTE: you should disable mailing to local mail (since this script echos text.)  To disable 
# local mailing, add the following to the top of your crontab:
#
# MAILTO=""
#
# Have fun!

tnlping="/root/bin/tnlping"
server="IP:67.18.11.66:25955"

sendoncefile="/root/bin/notified.txt"

to="<putemailhere@example.com>"

subject="The bitfighter server is down"
message="The bitfighter server is down at: $server"

# check tnlping existence
if [ ! -f "$tnlping" ]; then #error!
  echo "tnlping not found"
  exit 1
fi

# Do the ping
"$tnlping" "$server"

# mail if there was a problem
if [ $? == 1 ]; then
  # only mail if i haven't already
  if [ ! -f "$sendoncefile" ]; then
    echo "$message" | mail -s "$subject" "$to"
    echo "notified" > "$sendoncefile"
  fi
else
  if [ -f "$sendoncefile" ]; then
    rm "$sendoncefile"
  fi
fi

