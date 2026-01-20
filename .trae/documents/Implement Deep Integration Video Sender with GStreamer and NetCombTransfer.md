I will modify the `VideoSender` class to implement the deep integration of video transmission as requested.

### 1. Modify `VideoSender.h`

* Update the class definition to support the new sending mechanism.

* Add `destTID` member to store the destination Terminal ID.

* Add a static callback function `on_new_sample_from_sink` for the GStreamer `appsink` signal.

* Add a member function `handleRtpData` to process the extracted RTP packets and send them via `NetCombTransfer`.

### 2. Modify `VideoSender.cpp`

* **Pipeline Construction**: Replace the existing file-based pipeline with the requested hardware acceleration pipeline:
  `v4l2src ! nvv4l2h264enc ! rtph264pay mtu=1200 ! appsink name=rtp_sink emit-signals=true`

* **Data Extraction**: Implement the `on_new_sample_from_sink` callback to:

  * Pull the `GstSample` from `appsink`.

  * Map the `GstBuffer` to access raw data.

  * Call `handleRtpData`.

* **Network Injection**: Implement `handleRtpData` to:

  * Prepend a 1-byte header (`0x00` for RTP).

  * Wrap the data in `std::shared_ptr<BYTE>`.

  * Call `SendTIDData` from `NetCombTransferApi` to send the packet via UDP.

### 3. Verification

* Verify that the code compiles (conceptually, as I cannot run the full build).

* Ensure the pipeline string matches the specific reqirements (`mtu=1200`, `emit-signals=true`).

* Ensure the data extraction logic correctly handles memory mapping and cleanup.

