/********************************************************************************/
/*!
	@file			ili9342.c
	@author         Nemui Trinomius (http://nemuisan.blog.bai.ne.jp)
    @version        1.00
    @date           2013.01.02
	@brief          Based on Chan's MCI_OLED@LPC23xx-demo thanks!				@n
					It can drive XYL62291B-2B TFT module(8/16bit mode).

    @section HISTORY
		2013.01.02	V1.00	Stable Release

    @section LICENSE
		BSD License. See Copyright.txt
*/
/********************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "ili9342.h"
/* check header file version for fool proof */
#if __ILI9342_H != 0x0100
#error "header file version is not correspond!"
#endif

/* Defines -------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/

/* Constants -----------------------------------------------------------------*/

/* Function prototypes -------------------------------------------------------*/

/* Functions -----------------------------------------------------------------*/

/**************************************************************************/
/*! 
    Abstract Layer Delay Settings.
*/
/**************************************************************************/
#ifndef __SYSTICK_H
volatile uint32_t ticktime;
static inline void _delay_ms(uint32_t ms)
{
	ms += ticktime;
	while (ticktime < ms);
}
#endif

/**************************************************************************/
/*! 
    Display Module Reset Routine.
*/
/**************************************************************************/
inline void ILI9342_reset(void)
{
	ILI9342_RES_SET();							/* RES=H, RD=H, WR=H   		*/
	ILI9342_RD_SET();
	ILI9342_WR_SET();
	_delay_ms(10);								/* wait 10ms     			*/

	ILI9342_RES_CLR();							/* RES=L, CS=L   			*/
	ILI9342_CS_CLR();
	_delay_ms(1);								/* wait 1ms     			*/
	
	ILI9342_RES_SET();						  	/* RES=H					*/
	_delay_ms(10);				    			/* wait 10ms     			*/
}

/**************************************************************************/
/*! 
    Write LCD Command.
*/
/**************************************************************************/
inline void ILI9342_wr_cmd(uint8_t cmd)
{
	ILI9342_DC_CLR();							/* DC=L						*/

	ILI9342_CMD = cmd;							/* cmd						*/
	ILI9342_WR();								/* WR=L->H					*/

	ILI9342_DC_SET();							/* DC=H						*/
}

/**************************************************************************/
/*! 
    Write LCD Data.
*/
/**************************************************************************/
inline void ILI9342_wr_dat(uint8_t dat)
{
	ILI9342_DATA = dat;						/* data 					*/
	ILI9342_WR();								/* WR=L->H					*/
}

/**************************************************************************/
/*! 
    Write LCD GRAM.
*/
/**************************************************************************/
inline void ILI9342_wr_gram(uint16_t gram)
{
#if defined(GPIO_ACCESS_8BIT) | defined(BUS_ACCESS_8BIT)
	ILI9342_DATA = (uint8_t)(gram>>8);			/* upper 8bit data			*/
	ILI9342_WR();								/* WR=L->H					*/
	ILI9342_DATA = (uint8_t)gram;				/* lower 8bit data			*/
#else
	ILI9342_DATA = gram;						/* 16bit data 				*/
#endif
	ILI9342_WR();								/* WR=L->H					*/
}

/**************************************************************************/
/*! 
    Write LCD Block Data.
*/
/**************************************************************************/
inline void ILI9342_wr_block(uint8_t *p, unsigned int cnt)
{

#ifdef  USE_DISPLAY_DMA_TRANSFER
   DMA_TRANSACTION(p, cnt);
#else

	cnt /= 4;

	while (cnt--) {
		/* avoid -Wsequence-point's warning */
		ILI9342_wr_gram(*(p+1)|*(p)<<8);
		p++;p++;
		ILI9342_wr_gram(*(p+1)|*(p)<<8);
		p++;p++;
	}
#endif

}

/**************************************************************************/
/*! 
    Set Rectangle.
*/
/**************************************************************************/
inline void ILI9342_rect(uint32_t x, uint32_t width, uint32_t y, uint32_t height)
{

	ILI9342_wr_cmd(0x2A);				/* Horizontal RAM Start ADDR */
	ILI9342_wr_dat((OFS_COL + x)>>8);
	ILI9342_wr_dat(OFS_COL + x);
	ILI9342_wr_dat((OFS_COL + width)>>8);
	ILI9342_wr_dat(OFS_COL + width);

	ILI9342_wr_cmd(0x2B);				/* Horizontal RAM Start ADDR */
	ILI9342_wr_dat((OFS_RAW + y)>>8);
	ILI9342_wr_dat(OFS_RAW + y);
	ILI9342_wr_dat((OFS_RAW + height)>>8);
	ILI9342_wr_dat(OFS_RAW + height);

	ILI9342_wr_cmd(0x2C);				/* Write Data to GRAM */

}

/**************************************************************************/
/*! 
    Clear Display.
*/
/**************************************************************************/
inline void ILI9342_clear(void)
{
	volatile uint32_t n;

	ILI9342_rect(0,MAX_X-1,0,MAX_Y-1);
	n = (uint32_t)(MAX_X) * (MAX_Y);

	do {
		ILI9342_wr_gram(COL_BLACK);
	} while (--n);

}


/**************************************************************************/
/*! 
    Read LCD Register.
*/
/**************************************************************************/
inline uint16_t ILI9342_rd_cmd(uint8_t cmd)
{
	uint16_t val;
	uint16_t temp;

	ILI9342_wr_cmd(cmd);
	ILI9342_WR_SET();

    ReadLCDData(temp);							/* Dummy Read(Invalid Data) */
    ReadLCDData(temp);							/* Dummy Read				*/
    ReadLCDData(temp);							/* Upper Read				*/
    ReadLCDData(val);							/* Lower Read				*/

	val &= 0x00FF;
	val |= temp<<8;

	return val;
}


/**************************************************************************/
/*! 
    TFT-LCD Module Initialize.
*/
/**************************************************************************/
void ILI9342_init(void)
{
	uint16_t devicetype;

	Display_IoInit_If();

	ILI9342_reset();

	/* Set Manufacture command */
	ILI9342_wr_cmd(0xB9);
	ILI9342_wr_dat(0xFF);
	ILI9342_wr_dat(0x93);
	ILI9342_wr_dat(0x42);

	/* Check Device Code */
	devicetype = ILI9342_rd_cmd(0xD3);  			/* Confirm Vaild LCD Controller */

	if(devicetype == 0x9342)
	{
		/* Initialize ILI9342 */
		ILI9342_wr_cmd(0xB9);		/* Set Manufacture command */
		ILI9342_wr_dat(0xFF);
		ILI9342_wr_dat(0x93);
		ILI9342_wr_dat(0x42);

		ILI9342_wr_cmd(0x21);
		ILI9342_wr_cmd(0x36);
		ILI9342_wr_dat(0xC8);
		
		ILI9342_wr_cmd(0xC0);
		ILI9342_wr_dat(0x28);
		ILI9342_wr_dat(0x0A);
		
		ILI9342_wr_cmd(0xC1);
		ILI9342_wr_dat(0x02);
		
		ILI9342_wr_cmd(0xC5);
		ILI9342_wr_dat(0x2F);
		ILI9342_wr_dat(0x2F);
		
		ILI9342_wr_cmd(0xC7);
		ILI9342_wr_dat(0xC3);
		
		ILI9342_wr_cmd(0xB8);
		ILI9342_wr_dat(0x0B);
		
		ILI9342_wr_cmd(0xE0);
		ILI9342_wr_dat(0x0F);
		ILI9342_wr_dat(0x22);
		ILI9342_wr_dat(0x1D);
		ILI9342_wr_dat(0x0B);
		ILI9342_wr_dat(0x0F);
		ILI9342_wr_dat(0x07);
		ILI9342_wr_dat(0x4C);
		ILI9342_wr_dat(0x76);
		ILI9342_wr_dat(0x3C);
		ILI9342_wr_dat(0x09);
		ILI9342_wr_dat(0x16);
		ILI9342_wr_dat(0x07);
		ILI9342_wr_dat(0x12);
		ILI9342_wr_dat(0x0B);
		ILI9342_wr_dat(0x08);
		
		ILI9342_wr_cmd(0xE1);
		ILI9342_wr_dat(0x08);
		ILI9342_wr_dat(0x1F);
		ILI9342_wr_dat(0x24);
		ILI9342_wr_dat(0x03);
		ILI9342_wr_dat(0x0E);
		ILI9342_wr_dat(0x03);
		ILI9342_wr_dat(0x35);
		ILI9342_wr_dat(0x23);
		ILI9342_wr_dat(0x45);
		ILI9342_wr_dat(0x01);
		ILI9342_wr_dat(0x0B);
		ILI9342_wr_dat(0x07);
		ILI9342_wr_dat(0x2F);
		ILI9342_wr_dat(0x36);
		ILI9342_wr_dat(0x0F);

		ILI9342_wr_cmd(0x11);		/* Exit Sleep */
		_delay_ms(10);
		ILI9342_wr_cmd(0x11);		/* Exit Sleep */
		_delay_ms(80);

		ILI9342_wr_cmd(0x3A);		/* SET 65K COLOR */
		ILI9342_wr_dat(0x55);

		ILI9342_wr_cmd(0xB0);		/* SET EPL,DPL,VSL,HSL */
		ILI9342_wr_dat(0xe0);

		ILI9342_wr_cmd(0xf6);		/* SET Interface Control */
		ILI9342_wr_dat(0x01);
		ILI9342_wr_dat(0x00);
		ILI9342_wr_dat(0x00);

		ILI9342_wr_cmd(0x29);		/* Display ON */
	}

	else { for(;;);} /* Invalid Device Code!! */

	ILI9342_clear();

#if 0	/* test code RED */
	volatile uint32_t n;

	ILI9342_rect(0,MAX_X-1,0,MAX_Y-1);
	n = (uint32_t)(MAX_X) * (MAX_Y);

	do {
		ILI9342_wr_gram(COL_RED);
	} while (--n);
	
	_delay_ms(500);
	for(;;);
#endif

}


/**************************************************************************/
/*! 
    Draw Windows 24bitBMP File.
*/
/**************************************************************************/
int ILI9342_draw_bmp(const uint8_t* ptr){

	uint32_t n, m, biofs, bw, iw, bh, w;
	uint32_t xs, xe, ys, ye, i;
	uint8_t *p;
	uint16_t d;

	/* Load BitStream Address Offset  */
	biofs = LD_UINT32(ptr+10);
	/* Check Plane Count "1" */
	if (LD_UINT16(ptr+26) != 1)  return 0;
	/* Check BMP bit_counts "24(windows bitmap standard)" */
	if (LD_UINT16(ptr+28) != 24) return 0;
	/* Check BMP Compression "BI_RGB(no compresstion)"*/
	if (LD_UINT32(ptr+30) != 0)  return 0;
	/* Load BMP width */
	bw = LD_UINT32(ptr+18);
	/* Load BMP height */
	bh = LD_UINT32(ptr+22);
	/* Check BMP width under 1280px */
	if (!bw || bw > 1280 || !bh) return 0;
	
	/* Calculate Data byte count per holizontal line */
	iw = ((bw * 3) + 3) & ~3;

	/* Centering */
	if (bw > MAX_X) {
		xs = 0; xe = MAX_X-1;
	} else {
		xs = (MAX_X - bw) / 2;
		xe = (MAX_X - bw) / 2 + bw - 1;
	}
	if (bh > MAX_Y) {
		ys = 0; ye = MAX_Y-1;
	} else {
		ys = (MAX_Y - bh) / 2;
		ye = (MAX_Y - bh) / 2 + bh - 1;
	}

	/* Clear Display */
	ILI9342_clear();

    /* Limit to MAX_Y */
	m = (bh <= MAX_Y) ? biofs : biofs + (bh - MAX_Y) * iw;

	/* Set First data offset */  
	i = m % 512;

	/* Set BMP's Bottom-Left point */
	m = ye;

    /* Limit MAX_X */
	w = (bw > MAX_X) ? MAX_X : bw;

	do {
		ILI9342_rect(xs, xe, m, m);
		n = 0;
		p = (uint8_t*)ptr + i;
		do {
			n++;
			/* 65k colour access */
			d = *p++ >> 3;
			d |= (*p++ >> 2) << 5;
			d |= (*p++ >> 3) << 11;
			ILI9342_wr_gram(d);

		} while (n < w);
		i += iw;

	} while (m-- > ys);

	return 1;
}

/* End Of File ---------------------------------------------------------------*/
