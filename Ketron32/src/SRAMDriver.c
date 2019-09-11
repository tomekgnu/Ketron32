#include <avr/io.h>
#include "global.h"
#include "SRAMDriver.h"
#include "spi.h"
#include "string.h"
#include "stdint.h"

volatile uint8_t *sramDDR[SRAMChips] = {&DDRD,&DDRD,&DDRC,&DDRC};
volatile uint8_t *sramPORT[SRAMChips] = {&PORTD,&PORTD,&PORTC,&PORTC};
uint8_t sramPins[SRAMChips] = {PD5,PD6,PC0,PC1};

uint8_t SRAMBuf[SRAMPageSize];
uint8_t currentSRAM = SRAM_0;
sramAddress currentReader = {0};
sramAddress currentWriter = {0};
void incrementReader(int size);
void incrementWriter(int size);
void SRAM_resetReader();
void SRAM_resetWriter();


void SRAM_resetReader(){
	currentReader.currentByte.value = 0;
	currentReader.currentPage.value = 0;
	currentReader.totalBytes.value = 0;
	currentSRAM = 0;
}

void SRAM_resetWriter(){
	currentWriter.currentByte.value = 0;
	currentWriter.currentPage.value = 0;
	currentWriter.totalBytes.value = 0;
	currentSRAM = 0;
}

uint32_t SRAM_writerPosition(){
	return currentWriter.totalBytes.value;
}

uint32_t SRAM_readerPosition(){
	return currentReader.totalBytes.value;
}

void SRAM_seekRead(unsigned int size,unsigned int whence){
	switch(whence){
		case SRAM_SET: SRAM_resetReader();
					   incrementReader(size);
					   break;
		case SRAM_CUR: incrementReader(size);
					   break;
		case SRAM_END: SRAM_resetReader();
					   incrementReader(SRAMTotalSize - size);
					   break;
		default:	   break;

	}

}

void SRAM_seekWrite(unsigned int size,unsigned int whence){
	switch(whence){
		case SRAM_SET: SRAM_resetWriter();
					   incrementWriter(size);
					   break;
		case SRAM_CUR: incrementWriter(size);
					   break;
		case SRAM_END: SRAM_resetWriter();
					   incrementWriter(SRAMTotalSize - size);
					   break;
		default:	   break;

	}

}

uint32_t SRAM_written(){
	return currentWriter.totalBytes.value;
}

uint32_t SRAM_read(){
	return currentReader.totalBytes.value;
}

void incrementReader(int size){
	currentReader.totalBytes.value += size;
	if(currentReader.totalBytes.value >= SRAMTotalSize)
		currentReader.totalBytes.value %= SRAMTotalSize;
	currentReader.currentByte.value = currentReader.totalBytes.value % SRAMChipSize;
	currentReader.currentPage.value = (currentReader.totalBytes.value / SRAMPageSize) % SRAMPageCount;
	currentReader.currentSram = currentReader.totalBytes.value / SRAMChipSize;
	currentSRAM = currentReader.currentSram;
}

void incrementWriter(int size){
	currentWriter.totalBytes.value += size;
	if(currentWriter.totalBytes.value >= SRAMTotalSize)
		currentWriter.totalBytes.value %= SRAMTotalSize;
	currentWriter.currentByte.value = currentWriter.totalBytes.value % SRAMChipSize;
	currentWriter.currentPage.value = (currentWriter.totalBytes.value / SRAMPageSize) % SRAMPageCount;
	currentWriter.currentSram = currentWriter.totalBytes.value / SRAMChipSize;
	currentSRAM = currentWriter.currentSram;
}

void writeSRAM(unsigned char *buf,unsigned int size){
	unsigned int unaligned = currentWriter.currentByte.value % SRAMPageSize; // byte between start and end of page
	unsigned int remainder = (unaligned > 0?(SRAMPageSize - unaligned):0); // bytes remaining to end of page
	if(size == 0)
		return;

	if(size >= SRAMPageSize){
		if(unaligned == 0){
			// write page only, pass remaining size to next call
			SRAMWriteSeq(currentWriter.currentByte.bytes[0],currentWriter.currentByte.bytes[1],currentWriter.currentByte.bytes[2],buf,SRAMPageSize);
			incrementWriter(SRAMPageSize);
			writeSRAM(buf + SRAMPageSize,size - SRAMPageSize);
		}
		else{
			// write remainder, pass remaining size to next call
			SRAMWriteSeq(currentWriter.currentByte.bytes[0],currentWriter.currentByte.bytes[1],currentWriter.currentByte.bytes[2],buf,remainder);
			incrementWriter(remainder);
			writeSRAM(buf + remainder,size - remainder);
		}


	}
	else{

		if(unaligned > 0){
			if(size > remainder){
				size -= remainder;
				SRAMWriteSeq(currentWriter.currentByte.bytes[0],currentWriter.currentByte.bytes[1],currentWriter.currentByte.bytes[2],buf,remainder);
				incrementWriter(remainder);
				writeSRAM(buf + remainder,size);
			}
			else{
				SRAMWriteSeq(currentWriter.currentByte.bytes[0],currentWriter.currentByte.bytes[1],currentWriter.currentByte.bytes[2],buf,size);
				incrementWriter(size);
			}

		}
		else{
			SRAMWriteSeq(currentWriter.currentByte.bytes[0],currentWriter.currentByte.bytes[1],currentWriter.currentByte.bytes[2],buf,size);
			incrementWriter(size);
		}
	}
}


void readSRAM(unsigned char *buf,unsigned int size){
	unsigned int unaligned = currentReader.currentByte.value % SRAMPageSize; // byte between start and end of page
	unsigned int remainder = (unaligned > 0?(SRAMPageSize - unaligned):0); // bytes remaining to end of page

	if(size == 0)
		return;

	if(size >= SRAMPageSize){
		if(unaligned == 0){
			// read page only, pass remaining size to next call
			SRAMReadSeq(currentReader.currentByte.bytes[0],currentReader.currentByte.bytes[1],currentReader.currentByte.bytes[2],buf,SRAMPageSize);
			incrementReader(SRAMPageSize);
			readSRAM(buf + SRAMPageSize,size - SRAMPageSize);
		}
		else{
			// read remainder, pass remaining size to next call
			SRAMReadSeq(currentReader.currentByte.bytes[0],currentReader.currentByte.bytes[1],currentReader.currentByte.bytes[2],buf,remainder);
			incrementReader(remainder);
			readSRAM(buf + remainder,size - remainder);
		}


	}
	else{

		if(unaligned > 0){
			if(size > remainder){
				size -= remainder;
				SRAMReadSeq(currentReader.currentByte.bytes[0],currentReader.currentByte.bytes[1],currentReader.currentByte.bytes[2],buf,remainder);
				incrementReader(remainder);
				readSRAM(buf + remainder,size);
			}
			else{
				SRAMReadSeq(currentReader.currentByte.bytes[0],currentReader.currentByte.bytes[1],currentReader.currentByte.bytes[2],buf,size);
				incrementReader(size);
			}

		}
		else{
			SRAMReadSeq(currentReader.currentByte.bytes[0],currentReader.currentByte.bytes[1],currentReader.currentByte.bytes[2],buf,size);
			incrementReader(size);
		}
	}


}

BOOL checkSRAM(){
	uint8_t ok = 0;
	uint8_t data = 0;
	for(currentSRAM = SRAM_0; currentSRAM <= SRAM_3; currentSRAM++){
		SRAMWriteByte(0,0,0,'c');
		data = SRAMReadByte(0,0,0);
		if(data == 'c')
			ok++;
	}
	
	return ok == SRAMChips;
}


uint8_t ReadSPI(){
	return (uint8_t)spiTransferByte(DummyByte);
}

void WriteSPI(uint8_t byte){
	spiSendByte(byte);
}

void SRAMEnable(uint8_t n){
	cbi(*sramPORT[n],sramPins[n]);
	//HAL_GPIO_WritePin(sramPorts[n],sramPins[n],GPIO_PIN_RESET);
}

void SRAMDisable(uint8_t n){
	sbi(*sramPORT[n],sramPins[n]);
	//HAL_GPIO_WritePin(sramPorts[n],sramPins[n],GPIO_PIN_SET);
}

void InitSRAM(void)
{
	
	for(currentSRAM = SRAM_0; currentSRAM <= SRAM_3; currentSRAM++){
		sbi(*sramDDR[currentSRAM],sramPins[currentSRAM]);
		SRAMDisable(currentSRAM);
	}
}

uint8_t SRAMWriteStatusReg(uint8_t WriteVal)
{
	SRAMEnable(currentSRAM);
	WriteSPI(CMD_SRAMWRSR);
	//while(!SPI_Rx_Buf_Full);
	//ReadData = ReadSPI();
	WriteSPI(WriteVal);
	//while(!SPI_Rx_Buf_Full);
	//ReadData = ReadSPI();
	SRAMDisable(currentSRAM);
	return 0;			//Return non -ve nuber indicating success
}

uint8_t SRAMReadStatusReg(void)
{
	uint8_t ReadData;
	SRAMEnable(currentSRAM);
	WriteSPI(CMD_SRAMRDSR);
	//while(!SPI_Rx_Buf_Full);
	//ReadData = ReadSPI();
	WriteSPI(DummyByte);
	//while(!SPI_Rx_Buf_Full);
	ReadData = ReadSPI();
	SRAMDisable(currentSRAM);
	return ReadData;
}
void SRAMCommand(uint8_t AddLB, uint8_t AddMB,uint8_t AddHB, uint8_t RWCmd)
{
	//Send Read or Write command to SRAM
	WriteSPI(RWCmd);
	//while(!SPI_Rx_Buf_Full);
	//ReadData = ReadSPI();
	//Send High byte of address to SRAM
	WriteSPI(AddHB);
	WriteSPI(AddMB);
	//while(!SPI_Rx_Buf_Full);
	//ReadData = ReadSPI();
	//Send Low byte of address to SRAM
	WriteSPI(AddLB);
	//while(!SPI_Rx_Buf_Full);
	//ReadData = ReadSPI();
}

uint8_t SRAMWriteByte(uint8_t AddLB, uint8_t AddMB,uint8_t AddHB, uint8_t WriteData)
{
	SRAMWriteStatusReg(SRAMByteMode);
	SRAMEnable(currentSRAM);
	//Send Write command to SRAM along with address
	SRAMCommand(AddLB,AddMB,AddHB,CMD_SRAMWrite);
	//Send Data to be written to SRAM
	WriteSPI(WriteData);
	//while(!SPI_Rx_Buf_Full);
	//WriteData = ReadSPI();
	SRAMDisable(currentSRAM);
	return 0;			//Return non -ve number indicating success
}

uint8_t SRAMReadByte(uint8_t AddLB, uint8_t AddMB,uint8_t AddHB)
{
	uint8_t ReadData;
	SRAMWriteStatusReg(SRAMByteMode);
	SRAMEnable(currentSRAM);
	//Send Read command to SRAM along with address
	SRAMCommand(AddLB,AddMB,AddHB,CMD_SRAMRead);
	//Send dummy data so SRAM can put desired Data read from SRAM
	//WriteSPI(DummyByte);
	//while(!SPI_Rx_Buf_Full);
	ReadData = ReadSPI();
	SRAMDisable(currentSRAM);
	return ReadData;
}



uint32_t SRAMWritePage(uint8_t AddLB, uint8_t AddMB,uint8_t AddHB, uint8_t *WriteData)
{
	uint32_t WriteCnt;
	SRAMWriteStatusReg(SRAMPageMode);
	//Send Write command to SRAM along with address
	SRAMEnable(currentSRAM);
	SRAMCommand(AddLB,AddMB,AddHB,CMD_SRAMWrite);
	//Send Data to be written to SRAM
	for(WriteCnt = 0;WriteCnt < SRAMPageSize;WriteCnt++)
	{
		WriteSPI(*WriteData++);
		//while(!SPI_Rx_Buf_Full);
		//ReadData = ReadSPI();
	}
	SRAMDisable(currentSRAM);
	return WriteCnt;			//Return no# of bytes written to SRAM
}

uint32_t SRAMReadPage(uint8_t AddLB, uint8_t AddMB,uint8_t AddHB, uint8_t *ReadData)
{
	uint32_t ReadCnt;
	SRAMWriteStatusReg(SRAMPageMode);
	//Send Read command to SRAM along with address
	SRAMEnable(currentSRAM);
	SRAMCommand(AddLB,AddMB,AddHB,CMD_SRAMRead);
	//Send dummy data so SRAM can put desired Data read from SRAM
	for(ReadCnt = 0; ReadCnt < SRAMPageSize; ReadCnt++)
	{
		//while(!SPI_Rx_Buf_Full);
		*ReadData++ = ReadSPI();
	}
	SRAMDisable(currentSRAM);
	return ReadCnt;			//Return no# of bytes read from SRAM
}

uint8_t SRAMWriteSeq(uint8_t AddLB, uint8_t AddMB,uint8_t AddHB, uint8_t *WriteData,uint32_t WriteCnt)
{
	SRAMWriteStatusReg(SRAMSeqMode);
	//Send Write command to SRAM along with address
	SRAMEnable(currentSRAM);
	SRAMCommand(AddLB,AddMB,AddHB,CMD_SRAMWrite);
	//Send Data to be written to SRAM
	for(;WriteCnt > 0;WriteCnt--)
	{
		WriteSPI(*WriteData++);
		//while(!SPI_Rx_Buf_Full);
		//DummyRead =ReadSPI();
	}
	SRAMDisable(currentSRAM);
	return 0;			//Return non -ve nuber indicating success
}

uint8_t SRAMReadSeq(uint8_t AddLB, uint8_t AddMB,uint8_t AddHB, uint8_t *ReadData,uint32_t ReadCnt)
{
	SRAMWriteStatusReg(SRAMSeqMode);
	//Send Read command to SRAM along with address
	SRAMEnable(currentSRAM);
	SRAMCommand(AddLB,AddMB,AddHB,CMD_SRAMRead);
	//Send dummy data so SRAM can put desired Data read from SRAM
	for(; ReadCnt > 0; ReadCnt--)
	{
		//while(!SPI_Rx_Buf_Full);
		*ReadData++ = ReadSPI();
	}
	SRAMDisable(currentSRAM);
	return 0;			//Return non -ve nuber indicating success
}

