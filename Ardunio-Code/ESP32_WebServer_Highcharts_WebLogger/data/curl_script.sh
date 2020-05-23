#!/bin/sh
#----------------------------------------------------------
# W.Franke 2020 - CURL script to remotely log ESP32 Serial Web-Log-Lines
# run the script:  ./curl_script.sh  (tested on Raspi and in Mac OS 10.15)
# make it executable after saved first time:  chmod +x curl_script.sh
#----------------------------------------------------------
# make sure to use the IP-Address of your Web-Server (e.g. in a ESP32)
# you may rename the script with the name of the IP/device the script is CURLing
#----------------------------------------------------------
# What is the script doing for you:
# every second, CURL the Solar-Measure values from ESP32 web interface 
# the Script writes every received string into a log file on local hard disk
# at every new hour, a new log file is started
# used file name format in this script:  Name-20200515_112200.log
# means: hour is on position 10+11 to cut out from file date in name
# FYI: each second logging ~300 byte => ~1MB/h ; ~25MB/day ; ~1GB/month
#----------------------------------------------------------

# ! ! ! here make sure to use the IP-Address of your Web-Server ! ! !
myIPaddress=192.168.16.xxx
ServerPath="/curl_loglines_csv"

# change the log-file name to your convenience:
file_name_pre="demo"


#print start messsage in Terminal
echo "starting to curl $myIPaddress$ServerPath >> $file_name_pre-date_time.log (every second)"

#force main loop to compile an initial file name when starting the script
lastHour=25


#loop forever, until you hit CTRL-C , or close the terminal window
while :
do 
  #if next hour started then start a new log file
  if [ "$(date "+%H")" != "$lastHour" ]
  then
    #compile the new file name
    file_used_time=$(date "+%Y%m%d_%H%M%S")
    fileName=$file_name_pre-$file_used_time.log
    # save the current hour as last hour
    lastHour=$(echo $file_used_time | cut -c 10-11)
    #open a new log-file with this hours file name
    echo Start logging INA219 measures: $current_time > "$fileName"
  fi
  #write the received the data from web server int the log file
  #clear
  # CURL options: -s = silent mode / -S = still show error messages
  curl -s -S $myIPaddress$ServerPath >> "$fileName"
  sleep 1
done
