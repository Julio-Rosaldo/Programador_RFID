/*
 * ----------------------------------------------------------------------------
 * See https://github.com/miguelbalboa/rfid
 * for further details and other examples.
 * 
 * NOTE: The library file MFRC522.h has a lot of useful info. Please read it.
 * 
 * Released into the public domain.
 * ----------------------------------------------------------------------------
 * Sketch/program which will try the most used default keys listed in 
 * https://code.google.com/p/mfcuk/wiki/MifareClassicDefaultKeys to dump the
 * block 0 of a MIFARE RFID card using a RFID-RC522 reader.
 * 
 * Typical pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 *
 */

#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         9           // Configurable, see typical pin layout above
#define SS_PIN          10          // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

byte sector         = 1; // 0,    1,    2,      3,  ...
byte blockAddr      = 4; // 0*-2,  4-6,  8-10,   12-14, ...
byte trailerBlock   = 7; // 3,    7,    11,     15, ...

byte dataBlock[] = {
    'E', 'M', 'P', 'L', //  1,  2,   3,  4,
    'E', 'A', 'D', 'O', //  5,  6,   7,  8,
    'C', 'H', 'A', 'P', //  9, 10, 255, 11,
    'U', 'R', '6', '3'  // 12, 13, 14, 15
};

byte newKey[6] = {'J','C','R','P','9','4'};

// Number of known default keys (hard-coded)
// NOTE: Synchronize the NR_KNOWN_KEYS define with the defaultKeys[] array
#define NR_KNOWN_KEYS   9
// Known keys, see: https://code.google.com/p/mfcuk/wiki/MifareClassicDefaultKeys
byte knownKeys[NR_KNOWN_KEYS][MFRC522::MF_KEY_SIZE] =  {
    {'J','C','R','P','9','4'},
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, // FF FF FF FF FF FF = factory default
    {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5}, // A0 A1 A2 A3 A4 A5
    {0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5}, // B0 B1 B2 B3 B4 B5
    {0x4d, 0x3a, 0x99, 0xc3, 0x51, 0xdd}, // 4D 3A 99 C3 51 DD
    {0x1a, 0x98, 0x2c, 0x7e, 0x45, 0x9a}, // 1A 98 2C 7E 45 9A
    {0xd3, 0xf7, 0xd3, 0xf7, 0xd3, 0xf7}, // D3 F7 D3 F7 D3 F7
    {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff},  // AA BB CC DD EE FF
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  // 00 00 00 00 00 00
};


bool dataWrite = false;
bool keyWrite = false;

/*
 * Initialize.
 */
void setup() {
    Serial.begin(9600);         // Initialize serial communications with the PC
    while (!Serial);            // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
    SPI.begin();                // Init SPI bus
    mfrc522.PCD_Init();         // Init MFRC522 card

    //Serial.println(mfrc522.PCD_GetAntennaGain());
    //mfrc522.PCD_SetAntennaGain(0x07 << 4);
    //Serial.println(mfrc522.PCD_GetAntennaGain());

    Serial.println(F("Programador de tarjetas MIFARE Classic PICC"));
    Serial.println(F("Aplicacion escrita por el Ing. Julio Rosaldo"));
    Serial.println();

    Serial.print(F("ATENCION: Los datos existentes en el bloque #"));
    Serial.print(blockAddr);
    Serial.print(F(" del sector #"));
    Serial.print(sector);
    Serial.print(F(" seran sobreescritos."));
    Serial.println();

    for (int i=0; i<16; i++){
      Serial.write(" ");
      Serial.write(dataBlock[i]);
      Serial.write(" ");
    }
    Serial.println();
    for (int i=0; i<16; i++){
      Serial.print(dataBlock[i] < 0x10 ? " 0" : " ");
      Serial.print(dataBlock[i], HEX);
    }
    Serial.println();
    Serial.println();
    
    Serial.print(F("ATENCION: La llave del sector #"));
    Serial.print(sector);
    Serial.println(F(" sera sobreescrita."));
    
    for (int i=0; i<6; i++){
      Serial.write(" ");
      Serial.write(newKey[i]);
      Serial.write(" ");
    }
    Serial.println();
    for (int i=0; i<6; i++){
      Serial.print(newKey[i] < 0x10 ? " 0" : " ");
      Serial.print(newKey[i], HEX);
    }
    Serial.println();
    
    Serial.println();
    Serial.println();
}

/*
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

/*
 * Try using the PICC (the tag/card) with the given key to access block 0.
 * On success, it will show the key details, and dump the block data on Serial.
 *
 * @return true when the given key worked, false otherwise.
 */
bool try_key(MFRC522::MIFARE_Key *key)
{
    MFRC522::StatusCode status;
    byte buffer[18];
    byte size = sizeof(buffer);

    // Authenticate using key A
    Serial.println(F("Identificandose usando llave generica..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return false;
    }
    Serial.println(F("PCD_Authenticate() A success..."));
    Serial.println();

    // Show the whole sector as it currently is
    Serial.println(F("Datos existentes en el sector:"));
    mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), key, sector); // Funcion que imprime todos los datos de un sector
    Serial.println();

    // Read data from the block
    Serial.print(F("Leyendo informacion del bloque de datos #")); Serial.print(blockAddr); Serial.println(F(" ..."));
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size); // Funcion de lectura
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.print(F("Informacion encontrada:"));
    dump_byte_array(buffer, 16); Serial.println(); // Funcion que imprime los datos leidos del bloque
    Serial.println();


    // Read data from the trailer block
    Serial.print(F("Leyendo informacion del trailer block ")); Serial.print(trailerBlock); Serial.println(F(" ..."));
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(trailerBlock, buffer, &size); // Funcion de lectura
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }   
    byte dataTrailerBlock[16];
    for (byte i = 0; i < 16; i++)
    {
      dataTrailerBlock[i] = buffer[i];
    }
    Serial.println(F("Informacion encontrada:"));
    dump_byte_array(dataTrailerBlock, 16); Serial.println(); // Funcion que imprime los datos leidos del bloque
    Serial.println();


    // Preparar la nueva llave
    for (byte i = 0; i < 6; i++)
    {
      dataTrailerBlock[i] = newKey[i];
      dataTrailerBlock[i+10] = newKey[i];
    }
    Serial.println(F("Datos a escribir en el trailer block: "));
    dump_byte_array(dataTrailerBlock, 16); Serial.println(); // Funcion que imprime los datos leidos del bloque
    Serial.println();

    /*
    // Authenticate using key B
    Serial.println(F("Authenticating again using key B..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }
    Serial.println(F("PCD_Authenticate() B success..."));
    */

    
    // Escribir datos en el bloque seleccionado
    Serial.print(F("Escribiendo informacion en el bloque de datos #")); Serial.print(blockAddr); Serial.println(F(" ..."));  
    dump_byte_array(dataBlock, 16); Serial.println();
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.println(F("finalizado"));
    Serial.println();

    // Read data from the block (again, should now be what we have written)
    Serial.print(F("Leyendo informacion escrita en el bloque de datos #")); Serial.print(blockAddr); Serial.println(F(" ..."));    
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.println(F("Informacion encontrada:"));
    dump_byte_array(buffer, 16); Serial.println();
    Serial.println();

    // Check that data in block is what we have written
    // by counting the number of bytes that are equal
    Serial.println(F("Verificando integridad de los datos escritos..."));
    byte count = 0;
    for (byte i = 0; i < 16; i++) {
        // Compare buffer (= what we've read) with dataBlock (= what we've written)
        if (buffer[i] == dataBlock[i])
            count++;
    }
    Serial.print(F("Numero de bytes que coinciden = ")); Serial.println(count);
    if (count == 16) {
        Serial.println(F("Escritura de datos exitosa"));
        dataWrite = true;
    } else {
        Serial.println(F("ERROR: los datos no coinciden "));
    }
    Serial.println();


    // Escritura de datos en el bloque trailer seleccionado
    Serial.print(F("Escribiendo datos en el bloque trailer ")); Serial.print(trailerBlock);
    Serial.println(F(" ..."));
    dump_byte_array(dataTrailerBlock, 16); Serial.println();
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(trailerBlock, dataTrailerBlock, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.println(F("finalizado"));
    Serial.println();


    MFRC522::MIFARE_Key newKeyCheck;
    for (byte i = 0; i < MFRC522::MF_KEY_SIZE; i++) {
      newKeyCheck.keyByte[i] = newKey[i];
    }

    // TERMINA ESCRITURA
    

    // Authenticate using key A
    Serial.println(F("Identificandose usando la nueva llave..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &newKeyCheck, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return false;
    }
    else{
      keyWrite = true;
    }
    Serial.println(F("PCD_Authenticate() A success..."));
    Serial.println();


    // Dump the sector data
    Serial.println(F("Datos existentes en el sector:"));
    mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &newKeyCheck, sector);
    Serial.println();
    

    if (dataWrite == true && keyWrite == true){
      dataWrite = false;
      keyWrite = false;
      Serial.println(F("La escritura de los datos y la llave fue un exito :D"));
      Serial.println();
      Serial.println();
    }
    

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return true;
}

/*
 * Main loop.
 */
void loop() {
  delay(250);
    // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
    if ( ! mfrc522.PICC_IsNewCardPresent())
        return;

    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial())
        return;

    // Show some details of the PICC (that is: the tag/card)
    Serial.print(F("Card UID:"));
    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size); // UID DE LA TARJETA
    Serial.println();
    
    Serial.print(F("PICC type: "));
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak); // TIPO DE LA TARJETA
    Serial.println(mfrc522.PICC_GetTypeName(piccType));
    Serial.println();
    
    // Try the known default keys
    MFRC522::MIFARE_Key key;
    for (byte k = 0; k < NR_KNOWN_KEYS; k++) {
        // Copy the known key into the MIFARE_Key structure
        for (byte i = 0; i < MFRC522::MF_KEY_SIZE; i++) {
            key.keyByte[i] = knownKeys[k][i];
        }
        // PRUEBA LA LLAVE
        if (try_key(&key)) {
            // Found and reported on the key and block,
            // no need to try other keys for this PICC
            break;
        }

        // http://arduino.stackexchange.com/a/14316
        if ( ! mfrc522.PICC_IsNewCardPresent())
            break;
        if ( ! mfrc522.PICC_ReadCardSerial())
            break;
        
    }

    // Si la comunicacion falla la tarjeta se detiene, por lo que esta se reinicia...
    mfrc522.PICC_IsNewCardPresent();
    mfrc522.PICC_ReadCardSerial();
      
    // ...y se pone en modo de reposo para evitar lecturas repetivas
    mfrc522.PICC_HaltA(); // Se detiene el PICC
    mfrc522.PCD_StopCrypto1(); // Detiene la encriptacion en el PCD
}
