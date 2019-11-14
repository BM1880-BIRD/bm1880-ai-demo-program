# Stream Command Server

The stream command server is an example of using pipeline architecture that flexibly extends or overrides any functionality. It support a command stream (via tty) which can control server services, and fetch mutiple rtsp streams to perform face recognition and place results via network or tty.

### Config

Stream command server will setup with it's config.

* ```server_version [OPTIONAL]```: server version description.
* ```repository [NEED]```: where to save register face information.
* ```detector [NEED]```: face detector algorithm.
* ```detector_models [NEED]```: detector model's path.
* ```extractor [NEED]```: extractor algorithm.
* ```extractor_models [NEED]```: extractor model's path.
* ```similarity_threshold [NEED]```: face recognition threshold.
* ```command_disable [OPTIONAL]```: disabled command stream or not, if value is true, server won't setup command stream.

There can be many stream section in the configuration, and the stream section starts with ```stream_[name]```. In the stream section, you can configure as follows.

* ```stream_disable [OPTIONAL]```: configure this stream is disabled or not.
* ```auto_start [OPTIONAL]```: by default, stream start fetch when server run, and if you want to start stream manually, can set value to true.
* ```source_type [NEED]```: device/rtsp (default device (uvc camera)), decide source stream type.
* ```source_device_index [OPTIONAL]```: device index. ex: /dev/video0 index is 0.
* ```source_rtsp_host [OPTIONAL]```: rtsp server ip address.
* ```source_rtsp_port [OPTIONAL]```: rtsp server port.
* ```source_rtsp_url [OPTIONAL]```: rtsp url, the complete rtsp url is rtsp://rtsp_host:rtsp_port/rtsp_url.
* ```destination_type [OPTIONAL]```: http/tty (default tty), decide where to place face recognition results.
* ```destination_http_host [OPTIONAL]```: if destination type is http, used for destination http server ip address.
* ```destination_http_port [OPTIONAL]```: if destination type is http, used for destination http server port.
* ```destination_http_url [OPTIONAL]```: if destination type is http, used for destination http server url.

### Interface

The server provide basic functional interface, and if you need customize it, you can extended the stream command server and override any functionality.

* ```setConfigPath```: set config file path.
* ```addCmd```: add custom command to server's command stream, first parameter is the command name, and second parameter is the function called when the command is received. The function has ```tuple<string>``` type parameter and return value, the parameter is command value send by client, for example, client send ```start_stream stream_1```, ```start_stream``` is command name, and ```stream_1``` is command value, it will pass to function parameter. Return value is server response after handle this command.
* ```executeCmd```: execute server command.
* ```setAsyncCapacity```: set the asynchronous queue size, which is used for stream fetch and placement results.
* ```setContextValue```: set server's global context value, which is shared by all server's pipeline.
* ```getContextValue```: get server's global context value, which is shared by all server's pipeline.
* ```registerFace```: register face.
