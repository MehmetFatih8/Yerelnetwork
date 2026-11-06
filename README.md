# Yerelkeşif

A Windows program that scans your local network, lists the devices it finds, and lets you chat and send files to them. It uses async asio for all networking, so no extra threads are spent polling sockets. The UI is a WebView2 window with a custom frameless title bar.

## Usage

- Devices on the same LAN appear in the left panel automatically. Use **+ IP Ekle** to probe a specific address.
- Select a device to open its panel: drag files onto the drop zone (or use **+ Dosya Ekle**), then **Transferi Başlat**.
- Type in the chat box at the bottom; Enter sends, Shift+Enter adds a new line.
- Received files are saved to `Downloads\Yerelkesif`. Logs are written to `%TEMP%\!Yerelkesif_Logs`.

## Project layout

| Path | Role |
| --- | --- |
| `main.cpp` | Entry point: creates the controller, window and UI bridge |
| `AppController.*` | Owns the network stack, runs the io thread, keeps the device list |
| `FinderServer.*`, `FinderClient.*` | UDP discovery on port 8888 |
| `ServerSocket.*`, `PeerConnection.*` | Incoming TCP connections, frame decoding, file receive |
| `ClientSocket.*` | Outgoing TCP connections, message queue, file send queue |
| `Protocol.h` | Ports and wire prefixes |
| `NetEvents.h` | Events passed from the network layer to the controller |
| `Text.*` | UTF-8/UTF-16 and JSON helpers |
| `ui/MainWindow.*` | Frameless window, resize and shadow handling |
| `ui/UiBridge.*` | WebView2 setup, JS to C++ bindings, C++ to JS event forwarding |
| `ui/NativeFiles.*` | Native file picker and drag-and-drop handler |
| `ui/ArayuzHtml.h` | Front-end HTML, CSS and JavaScript |

## Wire protocol

| Port | Protocol | Messages |
| --- | --- | --- |
| UDP 8888 | discovery | `P2P_DISCOVER_HELLO:<user>` (broadcast or probe), `P2P_MATCH_HELLO:<user>` (reply) |
| TCP 9999 | main | `MSG:<byte length>:<text>` |
| TCP 9998 | file | `FILE_REQ:<name>:<bytes>:` then `FILE_ACK` or `FILE_ERR`, then raw file bytes |

A discovered device is connected on both TCP ports. Messages and files are sent over that outgoing connection.

## Build

Requires CMake 3.20+, a C++20 compiler (MinGW-w64 is tested) and internet access on the first configure, because webview and asio are fetched with FetchContent. The WebView2 SDK version is pinned in `CMakeLists.txt`.

Libraries: [webview/webview](https://github.com/webview/webview) 0.12.0 and [chriskohlhoff/asio](https://github.com/chriskohlhoff/asio).
