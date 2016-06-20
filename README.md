# Growina
Cloud Arduino Grow Controller using Wee
With the help of a few libraries, I have thrown together this little application 
It samples 4 tempratures and send it to the cloud.
And has 4 relays that can be set by 4 timers
The project needs a web page that accepts URL Query parameters and returns a command string to set the timers.
URL Query Ex: ?A0=20.1&A1=22.0&A2=22.0&A3=22.0&D0=1&D1=1&D2=0&D3=0
Ax are the Tempratures and Dx are the state of the relays
Command String Ex: #1100
Relays 1,2 is On ... 2,3 is off.

