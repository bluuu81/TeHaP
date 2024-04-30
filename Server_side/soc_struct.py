import socket
import struct
import time
import psycopg2
import sqlite3
from enum import Enum

class GPRS_send_types(Enum):
    STATUS = 20
    LOCALIZE = 21
    TEMP = 22
    PRESS = 23
    HUM = 24
    
def crc8(data):
    crc = 0xFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x80:
                crc = (crc << 1) ^ 0x31
            else:
                crc <<= 1
            crc &= 0xFF
    return crc


    return conn_db

def listen_on_port(port):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(('192.168.3.190', port))
    s.listen(1)
    print(f'Nasluchiwanie na porcie {port}...')
    while True:
        conn, addr = s.accept()
        print(f'Polaczenie nawiazane z {addr}')
        conn.settimeout(10.0)
        try:
            while True:
                # Odbierz dane od klienta
                data = conn.recv(1024)
                if not data:
                    break
                if data.startswith(b'\n'):
                    data = data[1:]
                    data += b'\x00'
                if len(data) >= 8:
                    token, UID, send_type = struct.unpack('<HIB', data[:7])
#                    print('Otrzymane dane: ',data)
#                    print('Dlugosc ramki: ', len(data))
                    if token != 0xDEF8:
                        print('Zły token')
                        break
                    print('UID:', format(UID, '08x'))
                    print('Ramka : ',send_type)
#                    print('Dane naglowka : ',data[:7])
                    if send_type == GPRS_send_types.STATUS.value:
                        MCU_temp, Vbat, Vac1, Vac2, charg_state_uu, timestamp, _, crc = struct.unpack('<fHHHBI9sB', data[7:32])                    
                        print('MCU Temp:', "{:.2f}".format(MCU_temp))
                        print('Vbat:', Vbat)
                        print('Vac1:', Vac1)
                        print('Vac2:', Vac2)
                        print('Charge State:', charg_state_uu)
                        print('Timestamp:', time.ctime(timestamp))
                        crc_srv = crc8(data[:-1])
                        if crc==crc_srv:
                            print('CRC OK')
                            crc_ok = True
                        else:
                            print('CRC NOT OK')
                            crc_ok = False
#                        print('CRC ramka:', format(crc, '02x'))
#                        print('CRC liczone:',crc8(data[:-1]))
                        save_to_status_db(UID, MCU_temp, Vbat, Vac1, Vac2, charg_state_uu, timestamp, crc_ok)
                    elif send_type == GPRS_send_types.LOCALIZE.value:                
                        lat, lon, sats, fix, timestamp, spare, crc = struct.unpack('<ffBBI10sB', data[7:32])
                        print('Latitude:', "{:.6f}".format(lat))
                        print('Longitude:', "{:.6f}".format(lon))
                        print('Satellites:', sats)
                        print('Fix:', fix)
                        print('Timestamp:', time.ctime(timestamp))
                        crc_srv = crc8(data[:-1])
                        if crc==crc_srv:
                            print('CRC OK')
                            crc_ok = True
                        else:
                            print('CRC NOT OK')
                            crc_ok = False
#                        print('CRC:', format(crc, '02x'))
#                        save_to_localize_db(format(UID, '08x'), send_type, "{:.6f}".format(lat), "{:.6f}".format(lon), sats, fix, time.ctime(timestamp), format(crc, '02x'))
                    elif send_type == GPRS_send_types.TEMP.value:
                        val1, val2, val3, anomaly_uu, timestamp, spare, crc = struct.unpack('<fffBI7sB', data[7:32])
                        print('Temp val1:', "{:.2f}".format(val1))
                        print('Temp val2:', "{:.2f}".format(val2))
                        print('Temp val3:', "{:.2f}".format(val3))
                        print('Timestamp:', time.ctime(timestamp))
                        crc_srv = crc8(data[:-1])
                        if crc==crc_srv:
                            print('CRC OK')
                            crc_ok = True
                        else:
                            print('CRC NOT OK')
                            crc_ok = False
#                        print('CRC:', format(crc, '02x'))
#                        save_to_temp_db(format(UID, '08x'), send_type, "{:.2f}".format(val1), "{:.2f}".format(val2), "{:.2f}".format(val3), time.ctime(timestamp), format(crc, '02x'))
                    elif send_type == GPRS_send_types.PRESS.value:
                        val1, val2, val3, anomaly_uu, timestamp, spare, crc = struct.unpack('<fffBI7sB', data[7:32])
                        print('Press val1:', "{:.2f}".format(val1))
                        print('Press val2:', "{:.2f}".format(val2))
                        print('Press val3:', "{:.2f}".format(val3))
                        print('Timestamp:', time.ctime(timestamp))
                        crc_srv = crc8(data[:-1])
                        if crc==crc_srv:
                            print('CRC OK')
                            crc_ok = True
                        else:
                            print('CRC NOT OK')
                            crc_ok = False
#                        print('CRC:', format(crc, '02x'))
#                        save_to_press_db(format(UID, '08x'), send_type, "{:.2f}".format(val1), "{:.2f}".format(val2), "{:.2f}".format(val3), time.ctime(timestamp), format(crc, '02x'))                        
                    elif send_type == GPRS_send_types.HUM.value:
                        val1, val2, val3, anomaly_uu, timestamp, spare, crc = struct.unpack('<fffBI7sB', data[7:32])
                        print('Hum val1:', "{:.2f}".format(val1))
                        print('Hum val2:', "{:.2f}".format(val2))
                        print('Hum val3:', "{:.2f}".format(val3))
                        print('Timestamp:', time.ctime(timestamp))
                        crc_srv = crc8(data[:-1])
                        if crc==crc_srv:
                            print('CRC OK')
                            crc_ok = True
                        else:
                            print('CRC NOT OK')
                            crc_ok = False
#                        print('CRC:', format(crc, '02x'))
#                        save_to_hum_db(format(UID, '08x'), send_type, "{:.2f}".format(val1), "{:.2f}".format(val2), "{:.2f}".format(val3), time.ctime(timestamp), format(crc, '02x'))
                    else:
                        print('Błąd: nieznany typ ramki')
                else:
                    print('Otrzymane dane:', data)
        except socket.timeout:
            print('Brak danych, timeout')
        finally:
            conn.close()

# Zapisywanie danych do tabeli StatusData
def save_to_status_db(uid, MCU_temp, Vbat, Vac1, Vac2, charg_state_uu, timestamp, crc_ok):
    cur.execute("""
        INSERT INTO public.devices (uid, uid)
        VALUES (%s, %s)
        ON CONFLICT (uid) DO NOTHING;
        """, (uid, uid, 'No remarks'))
    cur.execute("""
        INSERT INTO public.status_data (Mcu_temp, Vbat, Vac1, Vac2, charg_state_uu, timestamp, crc_ok)
                VALUES (%s, %s, %s, %s, %s, %s, %s);
                """, (mcu_temp, vbat, vac1, vac2, chrg_status, timestamp, crc_ok))
    cur.close()
    

# Połącz z PostgreSQL
conn_db = psycopg2.connect(
    dbname="thpbase",
    user="postgres",
    password="pass", #ChangeMe
    host="localhost"
    )
conn_db.autocommit = True
cur = conn_db.cursor()

listen_on_port(20390)
