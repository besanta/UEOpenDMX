
# Technical Details

* Include C++ Source for C++ Projects.

# UEOpenDMX
Implement DMX512 protocol so Unreal can send and receive DMX packages

# TODO

* WriteRange(ChannelBegin, ChannelEnd, Value);
* Clear(Value);

# See 
For Artnet implementation
https://github.com/besanta/UEArtnet

# Documentation
Initialize a DMX buffer Object, and connect it to a Serial COM device on the specified port.

/!\ The destructor of the buffer call the Close function so it releases the port for you, but keep in mind that the port is blocked if Close is not called.
![Alt Init](Documentation/BlueprintInit.png)

Update the buffer with some values.
![Alt Update](Documentation/BlueprintUpdate.png)

# Support
nicosanta@brightnightgames.net
