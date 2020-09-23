#ifndef __DODOBOT_SERIAL_H__
#define __DODOBOT_SERIAL_H__

#include <Arduino.h>

#define DATA_SERIAL  Serial5
#define INFO_SERIAL  Serial
#define SERIAL_MSG_BUFFER_SIZE 0x7f
char SERIAL_MSG_BUFFER[SERIAL_MSG_BUFFER_SIZE];
#define PACKET_START_0 '\x12'
#define PACKET_START_1 '\x34'
#define PACKET_STOP '\n'
#define PACKET_STOP_TIMEOUT 500

#define CHECK_SEGMENT(__SERIAL_OBJ__)  if (!__SERIAL_OBJ__->next_segment()) {  println_error("Not enough segments supplied for #%d: %s", __SERIAL_OBJ__->get_segment_num(), packet.c_str());  return;  }
#define CHECK_SEGMENT_BREAK(__SERIAL_OBJ__)  if (!__SERIAL_OBJ__->next_segment()) {  println_error("Not enough segments supplied for #%d: %s", __SERIAL_OBJ__->get_segment_num(), packet.c_str());  break;  }
#define DODOBOT_SERIAL_WRITE_BOTH(...)  dodobot_serial::data->write(__VA_ARGS__);  dodobot_serial::info->write(__VA_ARGS__);

namespace dodobot_serial
{
    enum PRINT_BUFFER_TYPES {
        PRINT_INFO,
        PRINT_ERROR
    };
    class DodobotSerial {
    public:

        DodobotSerial(void (*read_callback)(String, String)) {
            this->read_callback = read_callback;
            init_variables();
        }
        virtual bool ready() {  // override
            return false;
        }
        virtual Stream* device() {  // override
            return 0;
        }

        void write(String name, const char *formats, ...) {
            va_list args;
            va_start(args, formats);
            if (ready()) {
                make_packet(name, formats, args);
                device()->print(write_packet);
                write_packet_num++;
            }
            va_end(args);
        }
        void write(String packet) {
            if (!ready()) {
                return;
            }
            device()->print(packet);
        }
        void read() {
            if (!ready()) {
                return;
            }
            // 2 start chars
            // at least 1 char for packet num
            // \t + at least 1 category char
            // 2 chars for checksum
            // 1 new line
            if (device()->available() > 2) {
                char c;
                bool start_found = false;
                c = device()->read();
                if (c == PACKET_START_0) {
                    if (device()->available()) {
                        c = device()->read();
                        if (c == PACKET_START_1) {
                            start_found = true;
                        }
                    }
                }
                if (start_found) {
                    start_wait_time = millis();
                    while (true)
                    {
                        if (millis() - start_wait_time > PACKET_STOP_TIMEOUT) {
                            break;
                        }
                        if (device()->available()) {
                            c = device()->read();
                            if (c == PACKET_STOP) {
                                break;
                            }
                            recv_char_buffer[recv_char_index] = c;
                            recv_char_index++;
                        }
                    }
                    recv_char_buffer[recv_char_index] = '\0';
                    recv_char_index = 0;
                    read_packet = String(recv_char_buffer);
                    parse_packet();
                }
            }
        }
        bool next_segment()
        {
            if (read_packet_index >= read_packet.length()) {
                current_segment_num = -1;
                return false;
            }
            int separator = read_packet.indexOf('\t', read_packet_index);
            current_segment_num++;
            if (separator < 0) {
                segment = read_packet.substring(read_packet_index);
                read_packet_index = read_packet.length();
                return true;
            }
            else {
                segment = read_packet.substring(read_packet_index, separator);
                read_packet_index = separator + 1;
                return true;
            }
        }
        String get_segment() {
            return segment;
        }
        int get_segment_num() {
            return current_segment_num;
        }

        void print_buffer(PRINT_BUFFER_TYPES type, bool newline, const char* message) {
            if (!ready()) {
                return;
            }
            switch (type) {
                case PRINT_INFO:  device()->print("INFO\t"); break;
                case PRINT_ERROR:  device()->print("ERROR\t"); break;
                default: return;
            }
            device()->print(message);
            if (newline) {
                device()->print('\n');
            }
        }

        String get_written_packet() {
            return write_packet;
        }

    protected:
        String write_packet;
        String read_packet;
        String read_buffer;
        String segment;
        unsigned int read_packet_num;
        unsigned int write_packet_num;
        unsigned int buffer_index;
        unsigned int read_packet_index;
        int current_segment_num;
        bool prev_ready_state;
        char *recv_char_buffer;
        size_t recv_char_index;
        uint32_t start_wait_time;

        void init_variables() {
            write_packet = "";
            read_packet = "";
            read_buffer = "";
            segment = "";

            read_packet_num = 0;
            write_packet_num = 0;
            buffer_index = 0;
            read_packet_index = 0;
            current_segment_num = -1;
            prev_ready_state = false;

            recv_char_buffer = new char[0x800];
            recv_char_index = 0;

            start_wait_time = 0;
        }

        void (*read_callback)(String, String);
        void make_packet(String name, const char *formats, va_list args)
        {
            write_packet = String(PACKET_START_0) + String(PACKET_START_1);
            write_packet += String(write_packet_num) + "\t";
            write_packet += name;
            while (*formats != '\0') {
                write_packet += "\t";
                if (*formats == 'd') {
                    int i = va_arg(args, int32_t);
                    write_packet += String(i);
                }
                else if (*formats == 'u') {
                    uint32_t u = va_arg(args, uint32_t);
                    write_packet += String(u);
                }
                else if (*formats == 's') {
                    char *s = va_arg(args, char*);
                    write_packet += s;
                }
                else if (*formats == 'f') {
                    double f = va_arg(args, double);
                    write_packet += String(f);
                }
                else {
                    write("txrx", "dd", read_packet_num, 8);  // error 8: invalid format
                }
                ++formats;
            }
            // println_info("*packet: %s", *packet.c_str());

            uint8_t calc_checksum = 0;
            unsigned int length = write_packet.length();
            for (size_t index = 2; index < length; index++) {
                calc_checksum += (uint16_t)write_packet.charAt(index);
            }

            if (calc_checksum < 0x10) {
                write_packet += "0";
            }
            write_packet += String(calc_checksum, HEX);
            write_packet += String(PACKET_STOP);
        }

        /*char get_char() {
            char c = read_packet.charAt(read_packet_index);
            read_packet_index++;
            return c;
        }

        bool readline()
        {
            if (read_buffer.length() == 0) {
                return false;
            }
            int stop_index = read_buffer.indexOf('\n');
            if (stop_index == -1) {  // packet isn't ready. Come back later
                return false;
            }
            read_packet = read_buffer.substring(0, stop_index);
            read_packet_index = 0;
            read_buffer.remove(0, stop_index + 1);
            return true;
        }*/

        void parse_packet()
        {
            // read_packet assumes PACKET_START_0, PACKET_START_1, and PACKET_STOP have been removed

            /*char c1 = get_char();
            if (c1 != PACKET_START_0) {
                write("txrx", "dd", read_packet_num, 1);  // error 1: c1 != \x12
                return;
            }
            char c2 = get_char();
            if (c2 != PACKET_START_1) {
                write("txrx", "dd", read_packet_num, 2);  // error 2: c2 != \x34
                return;
            }

            read_packet.remove(0, 2);*/  // remove start characters

            read_packet_index = 0;
            // at least 1 char for packet num
            // \t + at least 1 category char
            // 2 chars for checksum
            if (read_packet.length() < 5) {
                write("txrx", "dd", read_packet_num, 3);  // error 3: packet is too short
                read_packet_num++;
                return;
            }

            // Calculate checksum
            uint8_t calc_checksum = 0;
            // compute checksum using all characters except the checksum itself
            for (size_t index = 0; index < read_packet.length() - 2; index++) {
                calc_checksum += (uint8_t)read_packet.charAt(index);
            }

            // extract checksum from packet
            uint8_t recv_checksum = strtol(read_packet.substring(read_packet.length() - 2).c_str(), NULL, 16);

            if (calc_checksum != recv_checksum) {
                // checksum failed
                write("txrx", "dd", read_packet_num, 4);  // error 4: checksums don't match
                read_packet_num++;
                return;
            }

            if (!next_segment()) {
                // failed to find packet num segment
                write("txrx", "dd", read_packet_num, 5);  // error 5: packet count segment not found
                read_packet_num++;
                return;
            }

            uint32_t recv_packet_num = segment.toInt();
            if (recv_packet_num != read_packet_num) {
                // this is considered a warning since it isn't critical for packet
                // numbers to be in sync
                write("txrx", "dd", read_packet_num, 6);  // error 6: packet counts not synchronized
                read_packet_num = recv_packet_num;
            }

            // find category segment
            if (!next_segment()) {
                write("txrx", "dd", read_packet_num, 7);  // error 7: failed to find category segment
                read_packet_num++;
                return;
            }
            String category = String(segment);

            // remove checksum
            read_packet.remove(read_packet.length() - 2, 2);
            (*read_callback)(category, read_packet);
            write("txrx", "dd", read_packet_num, 0);  // 0: no error
            read_packet_num++;
        }
    };

    class DodobotHWSerial : public DodobotSerial {
    private:
        HardwareSerial* __device;

    public:
        DodobotHWSerial (HardwareSerial* device, void (*read_callback)(String, String)) : DodobotSerial(read_callback) {
            __device = device;
        }
        Stream* device() {
            return __device;
        }
        bool ready() {
            return true;  // UART is always ready
        }
    };

    class DodobotUSBSerial : public DodobotSerial {
    private:
        usb_serial_class* __device;

    public:
        DodobotUSBSerial (usb_serial_class* device, void (*read_callback)(String, String)) : DodobotSerial(read_callback) {
            __device = device;
        }
        Stream* device() {
            return __device;
        }
        bool ready() {
            bool is_ready = (bool)__device->dtr();
            if (is_ready != prev_ready_state) {
                prev_ready_state = is_ready;
            }

            return is_ready;
        }
    };

    DodobotHWSerial* data;
    DodobotUSBSerial* info;

    void packet_callback(DodobotSerial* serial_obj, String category, String packet);

    void info_packet_callback(String category, String packet) {
        packet_callback(info, category, packet);
    }

    void data_packet_callback(String category, String packet) {
        packet_callback(data, category, packet);
    }

    // void println_info(const char* message)
    // {
    //     data->print_buffer(PRINT_INFO, true, message);
    //     info->print_buffer(PRINT_INFO, true, message);
    // }

    void println_info(const char* message, ...)
    {
        va_list args;
        va_start(args, message);
        vsnprintf(SERIAL_MSG_BUFFER, SERIAL_MSG_BUFFER_SIZE, message, args);
        va_end(args);
        data->print_buffer(PRINT_INFO, true, SERIAL_MSG_BUFFER);
        info->print_buffer(PRINT_INFO, true, SERIAL_MSG_BUFFER);
    }

    // void println_error(const char* message)
    // {
    //     data->print_buffer(PRINT_ERROR, true, message);
    //     info->print_buffer(PRINT_ERROR, true, message);
    // }

    void println_error(const char* message, ...)
    {
        va_list args;
        va_start(args, message);
        vsnprintf(SERIAL_MSG_BUFFER, SERIAL_MSG_BUFFER_SIZE, message, args);
        va_end(args);
        data->print_buffer(PRINT_ERROR, true, SERIAL_MSG_BUFFER);
        info->print_buffer(PRINT_ERROR, true, SERIAL_MSG_BUFFER);
    }

    // void println_info(const char* message)
    // {
    //     data->print_buffer(PRINT_INFO, false, message);
    //     info->print_buffer(PRINT_INFO, false, message);
    // }

    void print_info(const char* message, ...)
    {
        va_list args;
        va_start(args, message);
        vsnprintf(SERIAL_MSG_BUFFER, SERIAL_MSG_BUFFER_SIZE, message, args);
        va_end(args);
        data->print_buffer(PRINT_INFO, false, SERIAL_MSG_BUFFER);
        info->print_buffer(PRINT_INFO, false, SERIAL_MSG_BUFFER);
    }

    // void print_error(const char* message)
    // {
    //     data->print_buffer(PRINT_ERROR, false, message);
    //     info->print_buffer(PRINT_ERROR, false, message);
    // }

    void print_error(const char* message, ...)
    {
        va_list args;
        va_start(args, message);
        vsnprintf(SERIAL_MSG_BUFFER, SERIAL_MSG_BUFFER_SIZE, message, args);
        va_end(args);
        data->print_buffer(PRINT_ERROR, false, SERIAL_MSG_BUFFER);
        info->print_buffer(PRINT_ERROR, false, SERIAL_MSG_BUFFER);
    }

    void setup_serial()
    {
        DATA_SERIAL.begin(115200);
        INFO_SERIAL.begin(115200);

        // DATA_SERIAL.setTimeout(1000);
        // INFO_SERIAL.setTimeout(1000);
        // DATA_SERIAL.begin(500000);  // see https://www.pjrc.com/teensy/td_uart.html for UART info
        // while (!INFO_SERIAL) {
        //     delay(1);
        // }

        data = new DodobotHWSerial(&DATA_SERIAL, data_packet_callback);
        info = new DodobotUSBSerial(&INFO_SERIAL, info_packet_callback);

        println_info("Dobobot!");
        println_info("Serial buses initialized.");
    }
};  // namespace dodobot_serial

#endif // __DODOBOT_SERIAL_H__
