import socket
import struct
import time
import sqlite3
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
        conn.settimeout(15.0)
        try:
            while True:
                # Odbierz dane od klienta
                data = conn.recv(1024)
                if not data:
                    break
            
                if len(data) >= 10:
                    token, _, UID, send_type, _ = struct.unpack('<H2sIB3s', data[:12])
                    if token != 0xDEF8:
                        print('Zły token')
                        break
                    print('UID:', format(UID, '08x'))
                    if send_type == GPRS_send_types.STATUS.value:
                        MCU_temp, Vbat, Vac1, Vac2, charg_state, _, timestamp, crc, _  = struct.unpack('<fHHHB1sIB3s', data[12:32])
                        print('MCU Temp:', "{:.2f}".format(MCU_temp))
                        print('Vbat:', Vbat)
                        print('Vac1:', Vac1)
                        print('Vac2:', Vac2)
                        print('Charge State:', charg_state)
                        print('Timestamp:', time.ctime(timestamp))
                        print('CRC:', format(crc, '02x'))
                        save_to_status_db(format(UID, '08x'), send_type, "{:.2f}".format(MCU_temp), Vbat, Vac1, Vac2, charg_state, time.ctime(timestamp), format(crc, '02x'))
                    elif send_type == GPRS_send_types.LOCALIZE.value:                
                        lat, lon, sats, _, fix, _, timestamp, crc, _ = struct.unpack('<ffB1sB1sIB3s', data[12:32])
                        print('Latitude:', "{:.6f}".format(lat))
                        print('Longitude:', "{:.6f}".format(lon))
                        print('Satellites:', sats)
                        print('Fix:', fix)
                        print('Timestamp:', time.ctime(timestamp))
                        print('CRC:', format(crc, '02x'))
                        save_to_localize_db(format(UID, '08x'), send_type, "{:.6f}".format(lat), "{:.6f}".format(lon), sats, fix, time.ctime(timestamp), format(crc, '02x'))
                    elif send_type == GPRS_send_types.TEMP.value:
                        val1, val2, val3, timestamp, crc, _ = struct.unpack('<fffIB3s', data[12:32])
                        print('Temp val1:', "{:.2f}".format(val1))
                        print('Temp val2:', "{:.2f}".format(val2))
                        print('Temp val3:', "{:.2f}".format(val3))
                        print('Timestamp:', time.ctime(timestamp))
                        print('CRC:', format(crc, '02x'))
                        save_to_temp_db(format(UID, '08x'), send_type, "{:.2f}".format(val1), "{:.2f}".format(val2), "{:.2f}".format(val3), time.ctime(timestamp), format(crc, '02x'))
                    elif send_type == GPRS_send_types.PRESS.value:
                        val1, val2, val3, timestamp, crc, _ = struct.unpack('<fffIB3s', data[12:32])
                        print('Press val1:', "{:.2f}".format(val1))
                        print('Press val2:', "{:.2f}".format(val2))
                        print('Press val3:', "{:.2f}".format(val3))
                        print('Timestamp:', time.ctime(timestamp))
                        print('CRC:', format(crc, '02x'))
                        save_to_press_db(format(UID, '08x'), send_type, "{:.2f}".format(val1), "{:.2f}".format(val2), "{:.2f}".format(val3), time.ctime(timestamp), format(crc, '02x'))                        
                    elif send_type == GPRS_send_types.HUM.value:
                        val1, val2, val3, timestamp, crc, _ = struct.unpack('<fffIB3s', data[12:32])
                        print('Hum val1:', "{:.2f}".format(val1))
                        print('Hum val2:', "{:.2f}".format(val2))
                        print('Hum val3:', "{:.2f}".format(val3))
                        print('Timestamp:', time.ctime(timestamp))
                        print('CRC:', format(crc, '02x'))
                        save_to_hum_db(format(UID, '08x'), send_type, "{:.2f}".format(val1), "{:.2f}".format(val2), "{:.2f}".format(val3), time.ctime(timestamp), format(crc, '02x'))
                    else:
                        print('Błąd: nieznany typ ramki')
                else:
                    print('Otrzymane dane:', data)
        except socket.timeout:
            print('Brak danych, timeout')
        finally:
            conn.close()

# Zapisywanie danych do tabeli StatusData
def save_to_status_db(UID, send_type, MCU_temp, Vbat, Vac1, Vac2, charg_state, timestamp, crc):
    c.execute("INSERT INTO StatusData VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
              (UID, send_type, MCU_temp, Vbat, Vac1, Vac2, charg_state, timestamp, crc))
    conn_db.commit()

# Zapisywanie danych do tabeli LocalizeData
def save_to_localize_db(UID, send_type, lat, lon, sats, fix, timestamp, crc):
    c.execute("INSERT INTO LocalizeData VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
              (UID, send_type, lat, lon, sats, fix, timestamp, crc))
    conn_db.commit()

def save_to_temp_db(UID, send_type, val1, val2, val3, timestamp, crc):
    c.execute("INSERT INTO TempData VALUES (?, ?, ?, ?, ?, ?, ?)",
              (UID, send_type, val1, val2, val3, timestamp, crc))
    conn_db.commit()

def save_to_press_db(UID, send_type, val1, val2, val3, timestamp, crc):
    c.execute("INSERT INTO PressData VALUES (?, ?, ?, ?, ?, ?, ?)",
              (UID, send_type, val1, val2, val3, timestamp, crc))
    conn_db.commit()

def save_to_hum_db(UID, send_type, val1, val2, val3, timestamp, crc):
    c.execute("INSERT INTO HumData VALUES (?, ?, ?, ?, ?, ?, ?)",
              (UID, send_type, val1, val2, val3, timestamp, crc))
    conn_db.commit()


conn_db = sqlite3.connect('THP.db')
c = conn_db.cursor()
# Tworzenie tabeli dla STATUS
c.execute('''
    CREATE TABLE IF NOT EXISTS StatusData
    (UID text, SendType int, MCU_Temp real, Vbat real, Vac1 real, Vac2 real, Charge_State real, Timestamp text, CRC text)
''')

# Tworzenie tabeli dla LOCALIZE
c.execute('''
    CREATE TABLE IF NOT EXISTS LocalizeData
    (UID text, SendType int, Latitude real, Longitude real, Satellites int, Fix int, Timestamp text, CRC text)
''')

# Tworzenie tabeli dla TEMP
c.execute('''
    CREATE TABLE IF NOT EXISTS TempData
    (UID text, SendType int, Temp_val1 real, Temp_val2 real, Temp_val3 real, Timestamp text, CRC text)
''')

# Tworzenie tabeli dla PRESS
c.execute('''
    CREATE TABLE IF NOT EXISTS PressData
    (UID text, SendType int, Press_val1 real, Press_val2 real, Press_val3 real, Timestamp text, CRC text)
''')

# Tworzenie tabeli dla HUM
c.execute('''
    CREATE TABLE IF NOT EXISTS HumData
    (UID text, SendType int, Hum_val1 real, Hum_val2 real, Hum_val3 real, Timestamp text, CRC text)
''')




listen_on_port(20390)
