# Spotify-RFID-Record-Player
A lamp / record player utilising spotify's api and ESP8266 microcontroller

This is a project I started initially to learn how to make API requests and further my coding skills
it ended up being a pretty difficult challenge not because the api was hard to understand but there's not 
much useful documentation on making API calls with an ESP8266 online that I could find so a lot of
this project's time came down to doing research on making things work. The code itself is pretty easy
to understand in my opinion and should be easy to replicate, I'll try to leave all the versions of the 
IDE and libraries I used below as it turns out to be very important for anyone trying to replicate. 

The main function of this project is to use an ESP8266, and MFRC522 RFID reader, and and IR remote / sensor
to create a fully functioning lamp / a modern take on a record player using spotify's api. This project
can take any NFC / RFID card with a spotify URI uploaded to it and play that song to a desired device -
in this case it is a Google Home Nest Mini which is a Cast Audio Device, any other device would require
a change to the getDeviceID function. Full media controls are available via the IR remote, though keep in
mind the codes used in this example are to my IR remote, in order to find the codes to your remote I 
recommend simply printing out the "decode_results".value in HEX format to get your codes and don't forget
to put "0x" infront of it to get the proper functions working. 


Hardware Used In This Project:
  -ESP8266 by HiLetGo
    - https://www.amazon.com/HiLetgo-Internet-Development-Wireless-Micropython/dp/B010O1G1ES/ref=sr_1_2_sspa?crid=3FYQRIJF0933M&keywords=esp8266&qid=1662020809&sprefix=esp826%2Caps%2C143&sr=8-2-spons&th=1
  -MFRC522 RFID Sensor 
    - https://www.amazon.com/dp/B01CSTW0IA?psc=1&ref=ppx_yo2ov_dt_b_product_details
  -IR remote and sensor by Elegoo
    - (I used the remote and sensor from this kit but any sensor and remote should work as long as 
       as you're able to read the codes from it)
    - https://www.amazon.com/ELEGOO-Project-Tutorial-Controller-Projects/dp/B01D8KOZF4/ref=sr_1_2_sspa?crid=3M3SS3MHD8JMN&keywords=arduino&qid=1662020839&sprefix=arduino%2Caps%2C166&sr=8-2-spons&psc=1
  -NFC Cards
    - Any programmable NFC cards should work, look below for a tutorial on how to write to them
    - https://www.amazon.com/programmable-Compatible-Amiibo-Android-Enabled/dp/B0932T1QGC/ref=sr_1_3_sspa?crid=1UGB6QB6XIXM3&keywords=nfc%2Bcards&qid=1662020913&sprefix=nfc%2Bcards%2Caps%2C125&sr=8-3-spons&th=1
  -5mm RGB LEDS (4 leg)
    - This code uses 4 legs but any led rgb or not should work just fix the pin numbers 
      and amount of times you go through depending on which leds you use, this one just 
      comes with the correct 470 kohm resistors, if using a different pair find out which
      resistors to use and buy them, but the leds are the least important part of this whole project
    - https://www.amazon.com/dp/B077XGF3YR?psc=1&ref=ppx_yo2ov_dt_b_product_details
  
  
Library Versions:
  -ESP8266 board V.2.6.0
    -https://arduino.esp8266.com/stable/package_esp8266com_index.json
  -ArduinoJson by Benoit Blanchon V.6.19.3
  -IRremote by Armin Joachimsmeyer V.3.3.0
  -MFRC522 by GithubCommunity V.1.4.10
  
  
  Tutorial for how to write the URIs to the NFC cards:
  
    -Required materials:
      -Any Iphone or smartphone capable of reading and writing to NFC cards
      -Any programmable NFC cards, RFID won't work as Iphones only are capable
       of reading and writing to NFC chips
       
    -The video which I followed is below but I will also provide Written instructions
        -Video 
           - https://www.tiktok.com/@makeratplay/video/6926358046079716613?is_copy_url=0&is_from_webapp=v1&item_id=6926358046079716613
           
    -Written out steps:
      - 1. Download the app "NFC Tools" to your smartphone
          -Appstore
            - https://apps.apple.com/us/app/nfc-tools/id1252962749
          - Google Play
            - https://play.google.com/store/apps/details?id=com.wakdev.wdnfc&hl=en_US&gl=US
      
      - 2. Go to the spotify app and go the song you would like to upload
          - Spotify and this code isn't used for tracks, if you would like to use a single track
            just make a playlist and add that song to the playlist
          - Go the "Share" and copy the link of the song / album / playlist
          
      - 3. Go the NFC tools app and click on "Write"
      - 4. Click on "Add a Record"
      - 5. Click on URL / URI
      - 6. Paste in the spotify link you copied
          - delete the "https://" at the begining of the link you copied
          - the video says to delete anything after the question mark but I've had
            repeated success with just the "https://" deleted but if you have problems
            try deleting the junk after the question mark aswell
     - 7. Click on OK to go back and Click on "Write"
     - 8. Hold up the NFC card to the phone and the URI will be uploaded
     
     
 Finally Im going to put a link to "Maker At Play Coding"'s youtube channel and video which
 helped inspire and motivate me to make this project, his video uses and ESP32 and uses the 
 visual studio IDE which is better but a lot harder to work with especially for beginners, but
 his code is phonomenal and defenitely worth taking a look for, it's his code which I used to 
 read and write the URI for so huge thank you to him. Out of all the videos I watched trying to 
 find out how to make this project his was the most helpful by a mile. He hasn't uploaded in over a 
 year but still is worth checking out his videos to debug any code that I provided, not sure how 
 active he is anymore on any social medias but I'll float around and try to answer any questions 
 that may occur when trying to replicate the code
 
 Maker At Play Coding Channel: https://www.youtube.com/channel/UCUCydzw0QXIrNXi9NYUXgQA
 Maker At Play Coding - How to use Spotify API on ESP32 with NFC Reader to control Echo Dot: https://www.youtube.com/watch?v=RMtRH-3sTR4
  
  
  
  
  
