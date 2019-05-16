# UEOpenDMX
Implement DMX512 protocol so Unreal can send and receive DMX packages

![Alt wip](https://s3.amazonaws.com/snd-store/a/26553114/02_02_18_508408464_aab_560x292.jpg)

# Technical Details

* Include C++ Source for C++ Projects.

# TODO

* WriteRange(ChannelBegin, ChannelEnd, Value);
* Clear(Value);
* Support ArtNet

# Documentation
Initialize a DMX buffer, and connect it to a Serial COM device on the specified port.

/!\ The destructor of the buffer call the Close function so it releases the port for you, but keep in mind that the port is blocked if Close is not called.
![Alt Init](Documentation/BlueprintInit.png)

Update the buffer with some values.
![Alt Update](Documentation/BlueprintUpdate.png)

# Support
nicosanta@brightnightgames.net

# License
This is an open source project, you can use it freely. If you think this project useful, please give me a star to let me know it is useful, so I'll continue make it better.
