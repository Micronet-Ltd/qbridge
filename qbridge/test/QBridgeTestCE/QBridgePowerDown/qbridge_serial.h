#pragma once


namespace qbridge_serial {

	using namespace System;

		typedef enum ack_enums{
			ack_ok = '0',
			ack_duplicate_packet  = '1',
			ack_invalid_packet    = '2',
			ack_invalid_command   = '3',
			ack_invalid_data      = '4',
			ack_unable_to_process = '5',
			ack_can_bus_fault     = '6',
			ack_overflow_occurred = '7',
		};

		typedef enum debuginfotype{
			general = 0,
			j1708_log = 1,
			mid_info = 2
		};

		typedef enum getinfotype{
			buildinfo = 0,
			j1708info = 1,
			caninfo = 2,
			rst_ua = 3,	//reset the unit attention stuff pertaining to queue overflow
		};

	public ref class qbridgeserialcom	{
	public: 



		qbridgeserialcom(void);	//constructor
/*'@'*/		void init(void);
/*'A'*/		void ack(int id, ack_enums ackcode, unsigned char *data);
/*'B'*/		void mid_filter_enable(bool enable);
/*'C'*///	void set_mid( bool enable, unsigned char *midlist );
/*'D'*/		void send1708_packet( int packet_len, unsigned char *packet_info );
/*'E'*///	void receive1708_packet( unsigned char **packet_info );  //ONLY SENT BY THE QBRIDGE TO THE HOST
/*'F'*/		void enable_xmit_confirm( bool enable );
/*'G'*///	void xmit_confirm();	//ONLY SENT BY THE QBRIDGE TO THE HOST
/*'H'*/		void upgrade_firmware( System::Windows::Forms::ProgressBar ^progress );
/*'I'*/		void reset_qbridge( void );
/*'J'*/		void sendCANpacket( unsigned int id, int CANdlen, unsigned char *data );
/*'K'*///	void ReceiveCANpacket(  );	//ONLY SENT BY THE QBRIDGE TO THE HOST
/*'L'*/		void sendCANcontrol( int CANdlen, unsigned char *data );
/*'M'*/     void get_info( unsigned char *data );
/*'N'*///   void canbuserr();	//ONLY SENT BY THE QBRIDGE TO THE HOST
/*'*'*/		void info_request_to_debug_port(debuginfotype x);
/*'+'*///	void raw_1708( unsigned char *data );
/*','*///	void request_raw_packets( bool enable );
/*'-'*///	void echo( bool enable );
/*'p'*/		void pj( unsigned char *data );
			bool serialcom::test_if_in_bootloader_mode( void );
			char const*err;
			unsigned char *rdata;
			int status;	//bit0=in progress, bit1=ack with error bit2=
		void docmd( unsigned char cmd, unsigned char len, unsigned char *data );

		System::IO::Ports::SerialPort^  mySerial;
	protected:
		~qbridgeserialcom( void ); //destructor
	private:
		UInt32 *mycrctbl;
		unsigned char id;
		void getack( void );	//ack packet from "qbridge firmware"
		int getbl_ack( void );	//ack from "boot loader"

	};
};