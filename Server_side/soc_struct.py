import socket
import struct
import time
from enum import Enum

class GPRS_send_types(Enum):
    STATUS = 20
    LOCALIZE = 21
    TEMP = 22
    PRESS = 23
    HUM = 24

def listen_on_port(port):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(('192.168.3.190', port))
    s.listen(1)
    print(f'Nasluchiwanie na porcie {port}...')
    while True:
        conn, addr = s.accept()
        print(f'Polaczenie nawiazane z {addr}')
        while True:
            # Odbierz dane od klienta
            data = conn.recv(1024)
            if not data:
                break
            # Sprawdź, czy otrzymane dane są wystarczająco duże, aby zawierały strukturę GPRS_status_t
            print('Otrzymane dane:', data)
            
            if len(data) >= 10:
                token, _, UID, send_type, _ = struct.unpack('<H2sIB3s', data[:12])
                if token != 0xDEF8:
                    print('Zły token')
                    break
                print('Token:', format(token, '04x'))
                print('UID:', format(UID, '08x'))
                print('Send Type:', send_type)
                if send_type == GPRS_send_types.STATUS.value:
                    MCU_temp, Vbat, Vac1, Vac2, charg_state, _, timestamp, crc, _  = struct.unpack('<fHHHB1sIB3s', data[12:32])
                    print('MCU Temp:', "{:.2f}".format(MCU_temp))
                    print('Vbat:', Vbat)
                    print('Vac1:', Vac1)
                    print('Vac2:', Vac2)
                    print('Charge State:', charg_state)
                    print('Timestamp:', time.ctime(timestamp))
                    print('CRC:', format(crc, '02x'))
                elif send_type == GPRS_send_types.LOCALIZE.value:                
                    lat, lon, sats, _, fix, _, timestamp, crc, _ = struct.unpack('<ffB1sB1sIB3s', data[12:32])
                    print('Latitude:', "{:.6f}".format(lat))
                    print('Longitude:', "{:.6f}".format(lon))
                    print('Satellites:', sats)
                    print('Fix:', fix)
                    print('Timestamp:', time.ctime(timestamp))
                    print('CRC:', format(crc, '02x'))
                elif send_type == GPRS_send_types.TEMP.value:
                    val1, val2, val3, timestamp, crc, _ = struct.unpack('<fffIB3s', data[12:32])
                    print('Temp val1:', "{:.2f}".format(val1))
                    print('Temp val2:', "{:.2f}".format(val2))
                    print('Temp val3:', "{:.2f}".format(val3))
                    print('Timestamp:', time.ctime(timestamp))
                    print('CRC:', format(crc, '02x'))
#                elif send_type == GPRS_send_types.PRESS.value:
#                elif send_type == GPRS_send_types.HUM.value:
                else:
                    print('Błąd: nieznany typ ramki')
            else:
                print('Otrzymane dane:', data)
        conn.close()

listen_on_port(20390)
