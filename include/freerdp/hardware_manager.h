#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <syslog.h>
#include <sys/select.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <limits.h>
#include <restapi.h>
#include <freerdp/types.h>
#include <freerdp/utils/event_queue.h>
// #include <freerdp/utils/event_sender.h>
// #include <freerdp/virtual_interface.h>
#include <dal.h>
#include <capture_layer.h>
#include <freerdp/codec/rfx.h>



#ifndef __HW_MANAGER_H
#define __HW_MANAGER_H

// #include <memorymap.h>

//#define MULTIPLE_BUFFER //scheme introduced to try and free up core processing by signalling encode proceed before encryption starts

#ifdef MULTIPLE_BUFFER
	#define MAX_FRAME_BUFFERS 3
#else
	#define MAX_FRAME_BUFFERS 1
#endif

//#define READ_FPGA_ONCE
#define PARTIAL_FRAMES
//#define THROW_AWAY_MODE //the tile data is not read from the HIF and not sent over TCP, this feature is used for performance analysis
#define INTERUPT_DRIVEN  0
#define POLLING  1


#define  NOISE_FILTER_FILE "/usr/local/noise_filter.cfg"

#define MAX_FDS 32


#define USE_THREADING // turns on threading for mouse events
//#define MOUSE_RESPONSIVENESS_LOGGING


#define MODE INTERUPT_DRIVEN

#define CHAR_BITS	8

/*
	Need to know which pass through ....hw_manager_initialise_fpga() we are in (2 passes)
	- used in failure scenarios to pinpoint sync loss issue
*/
#define PASS1 1
#define PASS2 2

#ifndef LOBYTE
#define LOBYTE(a)		(unsigned char)((a) & (unsigned int)~0 >> CHAR_BITS)
#endif

#ifndef HIBYTE
#define HIBYTE(a)		(unsigned char)((unsigned int)(a) >> CHAR_BITS)
#endif

#define LONIBBLE(a)		(unsigned char)((a) & (unsigned char)0x0f)
#define HINIBBLE(a)		(unsigned char)(((unsigned char)(a)	>> 4) & 0x0f)
#define MAKE_INT_32(h,l) 	(UINT32)((((UINT32)(h)) << 16) | (UINT32)(l))
#define MAKE_INT_64(h,l) 	(uint64)((((uint64)(h)) << 32) | (uint64)(l))
#define MAKE_INT_16(h,l)   	(UINT16)((((UINT16)(h)) << 8) | (UINT16)(l))
#define MAKE_INT_8(h,l)   	(UINT8)(((UINT8)(h) << 4) | (UCHAR)(l & 0x0f))
#define LO_INT_16(a)		(UINT16)((UINT32)(a) & (UINT32)0xffff)
#define HI_INT_16(a)		(UINT16)((UINT32)(a) >> 16)
#define LO_INT_8(a)			(UINT8)((UINT16)(a) & (UINT16)0xff)
#define HI_INT_8(a)			(UINT8)((UINT16)(a) >> 8)

#define ABS(a)				(((a) < 0) ? (0 - (a)) : (a))

#define CHECK_BIT_64(var,pos) ((var & (1ULL << pos)) == (1ULL << pos))
#define REVERSE_UINT16(x) ((LOBYTE(x)<<8)|(HIBYTE(x)))
#define REVERSE_UINT32(x) (x >> 24) | ((x<<8) & 0x00FF0000) | ((x>>8) & 0x0000FF00) | (x<<24)


//#define DRAM_BUFFER // turn this off to use an non mmu controlled region of memory in dram for DMA transfer
#define FASTPATH_COPY //turn this on if you want the main data stream copied into the fast path stream, decreases performance
// but makes security header handling simpler
// Note: Since threading was introduced in June 2013 we need to use the FASTPATH_COPY method, failing to do so introduces a risk of message corruption when mouse, video and fastpath
// data are being sent, this is most evident at the start of a connection if mouse is being moved

#ifdef USE_THREADING
#define FASTPATH_COPY //we must use FASTPATH_COPY for threading
#endif

#define TILE_HEADER_SIZE 32
#define MAX_TILES 1181 //(64 X 18) + 30 == 1182 => (0 -- 1181)

#define DEV_MEM "/dev/mem"

#define JSON_BUFFER_SIZE 255
#define HW_DEFAULT_INTERVAL_PERIOD 1

#define MAX_FRAME_RECORDS 10
#define MAX_HEAD 2
#define HEAD_1 0
#define HEAD_2 1

enum hw_manager_state
{
	OFF = 0,
	ON = 1
};

typedef enum hw_media_state
{
	HEAD_ONE = 0,
	HEAD_TWO = 1,
	AUDIO = 2,
	USB = 3
} hwMediaState;

typedef enum thread_state {
	RUNNING,
	ENDING,
	STOPPED
} threadState;

typedef enum exit_state {
	NORMAL,
	ERROR
} exitState;

typedef struct frame_record FrameRecord;
struct frame_record
{
	UINT32 number_of_tiles;
	UINT32 frame_size;
	UINT32 processed_at;
	UINT32 head;
};

typedef struct
{
	videoHead_t ingress_resolution;	  	  //This is actual input resolution from the source
	videoHead_t optimised_egress_res;  	  //Connection resolution of Optimised Connections
	videoHead_t lossless_egress_res;	  //Connection resolution of Lossless Connections
	BOOL sync_loss;
} videoData_t;

typedef struct hw_manager_context hwManagerContext;

struct hw_manager_context
{
	// //------------------Memory Mapping -------------------------
    CAPTURE_LAYER_CONTEXT * capture_context;
	COMPRESSION_MODE configured_compression;

	//------------ Resolution -----------------------------------
	videoHead_t ingress_resolution[2];	  	  //This is actual input resolution from the source
	videoHead_t optimised_egress_res[2];  	  //Connection resolution of Optimised Connections
	videoHead_t lossless_egress_res;	      //Connection resolution of Lossless Connections
	BOOL optimised_path_scaled;				  //Set to true when Optimised path is scaled

	//------------ Board Config ---------------------------
	BOOL dual_head_board;

	//------------ Quantization ---------------------------
	UINT8 num_quants;
	UINT32* quants;
	UINT8 quant_idx_y;
	UINT8 quant_idx_cb;
	UINT8 quant_idx_cr;

	//------------ Noise Filter ---------------------------

	//noiseFilterSettings noise_filter_config_h1;


	//-------------FDs-----------------------------------------
	int krdm_fd; //fd for the kernel driver portion of fpga interface
	int audio_fd; //fd for the audio driver in the kernel
	int mouse_fd; //fd for the mouse driver in the kernel
	int secondary_mouse_fd; //fd for the mouse driver in the kernel
	int keyb_fd; //fd for the keyboard driver in the kernel
	BOOL enable_hid_tracing;

	// virtualInterface virtual_interface;
	//int virtual_fds[MAX_VIRTUAL_FDS]; //fds for virtual usb drivers in the kernel
	//int virtual_devices; // no of initialized virtual fds

	//---------------------------------Queues-----------------------
	eqEventQueue* hm_cm_queue;
	eqEventQueue* cm_hm_queue;
	eqEventQueue* hm_ep_queue;


	//-----------------------------------Threads----------------------
	pthread_t main_thread;
	threadState main_thread_state;

	//----------------------------------- status ----------------------
	exitState hm_exit_state;
	exitState ep_exit_state;
	char exit_info[255];
	//BOOL capture_enabled_h1; //FIXME do we actually need this
    BOOL head_detected[2];
	UINT32 receiver_head_count; //the client (receivers) head count
	BOOL fpga_reset_complete;
	//-----------------------------------For test and debug ---------------------------
	int id;
	BOOL performance_analysis;
	BOOL debug_enabled;
	// esContext * es_context;

	//----------------------------------- control ----------------------
	//-------------------------------------------------------------
	BOOL suspend_video_h1; //Do we need one per head?
	BOOL suspend_video_h2; //Do we need one per head?
	BOOL suspend_audio; //Do we need one per head?
	BOOL suspend_virtual; //For all virtual devices

	BOOL outputReportAvailable;
	UINT8 outputReportBitmask;
	BOOL signal_new_connection;
	UINT32 capture_rate;
	// hwQualityLevel quality_level;



	//--------------------------------------Mouse------------------------------
	//-------------------------------------------------------------------------
	int ab_x;
	int ab_y;
	int mouse_accel_enable; // ARPM: Acceleration enable
	int mouse_accel_offset;
	int mouse_accel_factor;
	int mouse_accel_type;
	int mouse_accel_subtype;
	double mouse_accel_x_factor;
	double mouse_accel_y_factor;
	double mouse_accel_A;
	double mouse_accel_B;
	double mouse_accel_C;
	int mouse_accel_auto_screen_res;
	int mouse_accel_x_screen_res;
	int mouse_accel_y_screen_res;



	//--------------------------------------Keyboard---------------------------
	//-------------------------------------------------------------------------
	int keys_held;


	//-----------------------------------Statistics---------------------------
	//------------------------------------------------------------------------
	VideoStatistics videoStatistics;
	// UsbAudioStatistics UsbaudioStatistics;
	// AnalogAudioStatistics analogaudioStatistics;
	// USBStatistics usbStatistics;

	UINT32 previous_frame_count;
	UINT32 previous_bytes_count;

	FrameRecord frame_records[MAX_FRAME_RECORDS];
	UINT32 frame_record_index;


	UINT32 previous_interval_time;
	UINT32 current_interval_time;

	//-------------------------------------------------------
	//Audio
	//-------------------------------------------------------
	BOOL audio_analog;
    BOOL enable_hm_heartbeats;
};

//Methods
//------------------------------------------
hwManagerContext*  hw_manager_new();
void hw_manager_free(hwManagerContext* context);
// void hw_manager_check_for_capture_rate_config(hwManagerContext * context);
// void hw_manager_signal_new_connection(hwManagerContext * context);
// void hw_manager_detect_resolution(hwManagerContext * context, int head);
void hw_manager_set_queues(hwManagerContext * context,eqEventQueue* hm_cm_queue,eqEventQueue* cm_hm_queue);
void hw_manager_run(hwManagerContext * context);
// void hw_manager_enable_performance_analysis(hwManagerContext * hm_context);
// void hw_manager_enable_debug(hwManagerContext * hm_context);
// void hw_manager_set_media_suspend_state(hwManagerContext * context,hwMediaState state,BOOL value);
// BOOL hw_manager_open_krdm(hwManagerContext * context);
// BOOL hw_manager_initialise_fpga(hwManagerContext * context,int head,int pass);
// void hw_manager_reset_and_configure_fpga(hwManagerContext * context, int head);
// void hw_manager_clear_video_head_structs(hwManagerContext * context, int head);
// float hw_manager_get_rolling_average (float avg, float new_sample,int statistic_type);
// void hw_manager_flush_pressed_keys(hwManagerContext * context);
// void hw_manager_analogaudio_stats(hwManagerContext *hw_context);
// void hardware_manager_enable_all_tiles_mode(hwManagerContext *hw_context, COMPRESSION_MODE compression_mode);
// void hardware_manager_init_capture_subsystem(hwManagerContext *hw_context, int head, COMPRESSION_MODE cm_compression_mode);
// void hardware_manager_start_capture_subsystem(hwManagerContext *hw_context, int head, COMPRESSION_MODE cm_compression_mode);
// void hardware_manager_stop_capture_subsystem(hwManagerContext *hw_context, int head, COMPRESSION_MODE cm_compression_mode);
// void hw_manager_video_stats(hwManagerContext *context, const UINT32 optimised_peers, const UINT32 lossless_peers);

// void hw_manager_cleanup(hwManagerContext *);
// void hw_manager_reset_fpga_logic(hwManagerContext  * context);
// void hw_manager_get_connection_resolution( hwManagerContext * context, const COMPRESSION_MODE compression, videoHead_t * connection_res, const video_head_index_e head );

#endif //__HW_MANAGER_H
