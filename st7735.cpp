#include "st7735.h"

namespace display_utils
{
	uint16_t ToRGB565(uint8_t R, uint8_t G, uint8_t B)
	{
		return ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
	}
}

ST7745::ST7745()
{

}

ST7745::~ST7745()
{

}

void ST7745::setMode(display_utils::SPIModes spi_mode)
{
	this->spi_mode = spi_mode;
	switch (spi_mode)
	{
	case display_utils::SPIModes::Soft:
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
		GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE); // Disable JTAG for use PB3
		break;
	case display_utils::SPIModes::First:
		spi_port = SPI1;
		spi_sck_pin = GPIO_Pin_5;
		spi_mosi_pin = GPIO_Pin_7;
		spi_gpio_port = GPIOA;
		break;
	case display_utils::SPIModes::Second:
		spi_port = SPI2;
		spi_sck_pin = GPIO_Pin_13;
		spi_mosi_pin = GPIO_Pin_15;
		spi_gpio_port = GPIOB;
		break;
	case::display_utils::SPIModes::Third:
		spi_port = SPI3;
		spi_sck_pin = GPIO_Pin_3;
		spi_mosi_pin = GPIO_Pin_5;
		spi_gpio_port = GPIOB;
		break;
	}
}

inline void ST7745::cs_l()
{
	GPIO_ResetBits(cs_port, cs_pin);
}

inline void ST7745::cs_h()
{
	GPIO_SetBits(cs_port, cs_pin);
}

inline void ST7745::rst_l()
{
	GPIO_ResetBits(rst_port, rst_pin);
}

inline void ST7745::rst_h()
{
	GPIO_SetBits(rst_port, rst_pin);
}

inline void ST7745::a0_l()
{
	GPIO_ResetBits(a0_port, a0_pin);
}

inline void ST7745::a0_h()
{
	GPIO_SetBits(a0_port, a0_pin);
}

void ST7745::write(uint8_t data)
{
	if (this->spi_mode == display_utils::Soft) {
//		uint8_t i;
//
//		for (i = 0; i < 8; i++)
//		{
//			if (data & 0x80) SDA_H();
//			else SDA_L();
//			data = data << 1;
//			SCK_L();
//			SCK_H();
//		}
	} else
	{
		while (SPI_I2S_GetFlagStatus(spi_port, SPI_I2S_FLAG_TXE) == RESET)
			;
		SPI_I2S_SendData(spi_port, data);
	}
}

void ST7745::cmd(uint8_t cmd)
{
	a0_l();
	write(cmd);
	if (this->spi_mode != display_utils::Soft) {
		while (SPI_I2S_GetFlagStatus(spi_port, SPI_I2S_FLAG_BSY) == SET)
			;
	}
}

void ST7745::data(uint8_t data)
{
	a0_h();
	write(data);
	if (this->spi_mode != display_utils::Soft) {
		while (SPI_I2S_GetFlagStatus(spi_port, SPI_I2S_FLAG_BSY) == SET)
			;
	}
}

void ST7745::orientation(display_utils::ScrOrientation_TypeDef orientation)
{
	cs_l();
	cmd(0x36); // Memory data access control:
	uint16_t swap = 0;
	switch (orientation)
	{
	case scr_CW:
		swap = scr_width;
		scr_width = scr_height;
		scr_height = swap;
		data(0xA0); // X-Y Exchange,Y-Mirror
		break;
	case scr_CCW:
		swap = scr_width;
		scr_width = scr_height;
		scr_height = swap;
		data(0x60); // X-Y Exchange,X-Mirror
		break;
	case scr_180:
		data(0xc0); // X-Mirror,Y-Mirror: Bottom to top; Right to left; RGB
		break;
	default:
		data(0x00); // Normal: Top to Bottom; Left to Right; RGB
		break;
	}
	cs_h();
}

void ST7745::init()
{
	switch (spi_mode)
	{
	case display_utils::SPIModes::Soft:
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
		GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE); // Disable JTAG for use PB3
		break;
	case display_utils::SPIModes::First:
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1 | RCC_APB2Periph_GPIOA, ENABLE);
		break;
	case display_utils::SPIModes::Second:
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
		break;
	case::display_utils::SPIModes::Third:
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
		GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE); // Disable JTAG for use PB3
		break;
	}

	//Not implemented fully yet.
	if(spi_mode != display_utils::SPIModes::Soft)
	{
		SPI_InitTypeDef SPI;
		SPI.SPI_Mode = SPI_Mode_Master;
		SPI.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
		SPI.SPI_Direction = SPI_Direction_1Line_Tx;
		SPI.SPI_CPOL = SPI_CPOL_Low;
		SPI.SPI_CPHA = SPI_CPHA_1Edge;
		SPI.SPI_CRCPolynomial = 7;
		SPI.SPI_DataSize = SPI_DataSize_8b;
		SPI.SPI_FirstBit = SPI_FirstBit_MSB;
		SPI.SPI_NSS = SPI_NSS_Soft;
		SPI_Init(spi_port, &SPI);
		// NSS must be set to '1' due to NSS_Soft settings (otherwise it will be Multimaster mode).
		SPI_NSSInternalSoftwareConfig(spi_port, SPI_NSSInternalSoft_Set);
		SPI_Cmd(spi_port, ENABLE);
	}


	GPIO_InitTypeDef PORT;
	PORT.GPIO_Mode = GPIO_Mode_Out_PP;
	PORT.GPIO_Speed = GPIO_Speed_50MHz;

	if (spi_mode == display_utils::SPIModes::Soft)
	{
//		PORT.GPIO_Pin = ST7735_SDA_PIN;
//		GPIO_Init(ST7735_SDA_PORT, &PORT);
//		PORT.GPIO_Pin = ST7735_SCK_PIN;
//		GPIO_Init(ST7735_SCK_PORT, &PORT);
	} else
	{
		PORT.GPIO_Pin = spi_sck_pin | spi_mosi_pin;
		PORT.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_Init(spi_gpio_port, &PORT);
		PORT.GPIO_Mode = GPIO_Mode_Out_PP;
	}


	RCC_APB2PeriphClockCmd(a0_port_clk | rst_port_clk | cs_port_clk, ENABLE);
	PORT.GPIO_Pin = cs_pin;
	GPIO_Init(cs_port, &PORT);
	PORT.GPIO_Pin = rst_pin;
	GPIO_Init(rst_port, &PORT);
	PORT.GPIO_Pin = a0_pin;
	GPIO_Init(a0_port, &PORT);

	// Reset display
	cs_h();
	rst_h();
	Delay_US(5);
	rst_l();
	Delay_US(5);
	rst_h();
	cs_h();
	Delay_US(5);
	cs_l();

	cmd(0x11);     // Sleep out, booster on
	Delay_US(20);

	cmd(0xb1);     // In normal mode (full colors):
	a0_h();
	write(0x05);   //   RTNA set 1-line period: RTNA2, RTNA0
	write(0x3c);   //   Front porch: FPA5,FPA4,FPA3,FPA2
	write(0x3c);   //   Back porch: BPA5,BPA4,BPA3,BPA2

	cmd(0xb2);     // In idle mode (8-colors):
	a0_h();
	write(0x05);   //   RTNB set 1-line period: RTNAB, RTNB0
	write(0x3c);   //   Front porch: FPB5,FPB4,FPB3,FPB2
	write(0x3c);   //   Back porch: BPB5,BPB4,BPB3,BPB2

	cmd(0xb3);     // In partial mode + full colors:
	a0_h();
	write(0x05);   //   RTNC set 1-line period: RTNC2, RTNC0
	write(0x3c);   //   Front porch: FPC5,FPC4,FPC3,FPC2
	write(0x3c);   //   Back porch: BPC5,BPC4,BPC3,BPC2
	write(0x05);   //   RTND set 1-line period: RTND2, RTND0
	write(0x3c);   //   Front porch: FPD5,FPD4,FPD3,FPD2
	write(0x3c);   //   Back porch: BPD5,BPD4,BPD3,BPD2

	cmd(0xB4);     // Display dot inversion control:
	data(0x03);    //   NLB,NLC

	cmd(0x3a);     // Interface pixel format
				   //ST7735_data(0x03);    // 12-bit/pixel RGB 4-4-4 (4k colors)
	data(0x05);    // 16-bit/pixel RGB 5-6-5 (65k colors)
						  //ST7735_data(0x06);    // 18-bit/pixel RGB 6-6-6 (256k colors)

						  //ST7735_cmd(0x36);     // Memory data access control:
						  // MY MX MV ML RGB MH - -
						  //ST7735_data(0x00);    // Normal: Top to Bottom; Left to Right; RGB
						  //ST7735_data(0x80);    // Y-Mirror: Bottom to top; Left to Right; RGB
						  //ST7735_data(0x40);    // X-Mirror: Top to Bottom; Right to Left; RGB
						  //ST7735_data(0xc0);    // X-Mirror,Y-Mirror: Bottom to top; Right to left; RGB
						  //ST7735_data(0x20);    // X-Y Exchange: X and Y changed positions
						  //ST7735_data(0xA0);    // X-Y Exchange,Y-Mirror
						  //ST7735_data(0x60);    // X-Y Exchange,X-Mirror
						  //ST7735_data(0xE0);    // X-Y Exchange,X-Mirror,Y-Mirror

	cmd(0x20);     // Display inversion off
						  //ST7735_cmd(0x21);     // Display inversion on

	cmd(0x13);     // Partial mode off

	cmd(0x26);     // Gamma curve set:
	data(0x01);    // Gamma curve 1 (G2.2) or (G1.0)
						  //ST7735_data(0x02);    // Gamma curve 2 (G1.8) or (G2.5)
						  //ST7735_data(0x04);    // Gamma curve 3 (G2.5) or (G2.2)
						  //ST7735_data(0x08);    // Gamma curve 4 (G1.0) or (G1.8)

	cmd(0x29);     // Display on
	cs_h();
	orientation(display_utils::scr_normal);
}

void ST7745::addrSet(uint16_t XS, uint16_t YS, uint16_t XE, uint16_t YE)
{
	cmd(0x2a); // Column address set
	a0_h();
	write(XS >> 8);
	write(XS);
	write(XE >> 8);
	write(XE);
	cmd(0x2b); // Row address set
	a0_h();
	write(YS >> 8);
	write(YS);
	write(YE >> 8);
	write(YE);
	cmd(0x2c); // Memory write
}

void ST7745::clear(uint16_t color)
{
	uint8_t  CH, CL;

	CH = color >> 8;
	CL = (uint8_t)color;

	cs_l();
	addrSet(0, 0, scr_width - 1, scr_height - 1);
	a0_h();
	for (uint16_t i = 0; i < scr_width * scr_height; i++)
	{
		write(CH);
		write(CL);
	}
	cs_h();
}

void ST7745::setPixel(uint16_t X, uint16_t Y, uint16_t color)
{
	cs_l();
	addrSet(X, Y, X, Y);
	a0_h();
	write(color >> 8);
	write((uint8_t)color);
	cs_h();
}

void ST7745::hLine(uint16_t X1, uint16_t X2, uint16_t Y, uint16_t color)
{
	uint8_t CH = color >> 8;
	uint8_t CL = (uint8_t)color;

	cs_l();
	addrSet(X1, Y, X2, Y);
	a0_h();
	for (uint16_t i = 0; i <= (X2 - X1); i++)
	{
		write(CH);
		write(CL);
	}
	cs_h();
}

void ST7745::vLine(uint16_t X, uint16_t Y1, uint16_t Y2, uint16_t color)
{
	uint8_t CH = color >> 8;
	uint8_t CL = (uint8_t)color;

	cs_l();
	addrSet(X, Y1, X, Y2);
	a0_h();
	for (uint16_t i = 0; i <= (Y2 - Y1); i++)
	{
		write(CH);
		write(CL);
	}
	cs_h();
}


void ST7745::line(int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint16_t color)
{
	int16_t dX = X2 - X1;
	int16_t dY = Y2 - Y1;
	int16_t dXsym = (dX > 0) ? 1 : -1;
	int16_t dYsym = (dY > 0) ? 1 : -1;

	if (dX == 0)
	{
		if (Y2 > Y1) vLine(X1, Y1, Y2, color);
		else vLine(X1, Y2, Y1, color);
		return;
	}
	if (dY == 0)
	{
		if (X2 > X1) hLine(X1, X2, Y1, color);
		else hLine(X2, X1, Y1, color);
		return;
	}

	dX *= dXsym;
	dY *= dYsym;
	int16_t dX2 = dX << 1;
	int16_t dY2 = dY << 1;
	int16_t di;

	if (dX >= dY)
	{
		di = dY2 - dX;
		while (X1 != X2)
		{
			setPixel(X1, Y1, color);
			X1 += dXsym;
			if (di < 0)
			{
				di += dY2;
			}
			else
			{
				di += dY2 - dX2;
				Y1 += dYsym;
			}
		}
	}
	else
	{
		di = dX2 - dY;
		while (Y1 != Y2)
		{
			setPixel(X1, Y1, color);
			Y1 += dYsym;
			if (di < 0)
			{
				di += dX2;
			}
			else
			{
				di += dX2 - dY2;
				X1 += dXsym;
			}
		}
	}
	setPixel(X1, Y1, color);
}


void ST7745::rect(uint16_t X1, uint16_t Y1, uint16_t X2, uint16_t Y2, uint16_t color)
{
	hLine(X1, X2, Y1, color);
	hLine(X1, X2, Y2, color);
	vLine(X1, Y1, Y2, color);
	vLine(X2, Y1, Y2, color);
}

void ST7745::fillRect(uint16_t X1, uint16_t Y1, uint16_t X2, uint16_t Y2, uint16_t color)
{
	uint16_t FS = (X2 - X1 + 1) * (Y2 - Y1 + 1);
	uint8_t CH = color >> 8;
	uint8_t CL = (uint8_t)color;

	cs_l();
	addrSet(X1, Y1, X2, Y2);
	a0_h();
	for (uint16_t i = 0; i < FS; i++)
	{
		write(CH);
		write(CL);
	}
	cs_h();
}

void ST7745::putChar5x7(uint8_t scale, uint16_t X, uint16_t Y, uint8_t chr, uint16_t color, uint16_t bgcolor)
{
	uint16_t i, j;
	uint8_t buffer[5];
	uint8_t CH = color >> 8;
	uint8_t CL = (uint8_t)color;
	uint8_t BCH = bgcolor >> 8;
	uint8_t BCL = (uint8_t)bgcolor;

	if ((chr >= 0x20) && (chr <= 0x7F))
	{
		// ASCII[0x20-0x7F]
		memcpy(buffer, &Font5x7[(chr - 32) * 5], 5);
	}
	else if (chr >= 0xA0)
	{
		// CP1251[0xA0-0xFF]
		memcpy(buffer, &Font5x7[(chr - 64) * 5], 5);
	}
	else
	{
		// unsupported symbol
		memcpy(buffer, &Font5x7[160], 5);
	}

	cs_l();

	// scale equals 1 drawing faster
	if (scale == 1)
	{
		addrSet(X, Y, X + 5, Y + 7);
		a0_h();
		for (j = 0; j < 7; j++)
		{
			for (i = 0; i < 5; i++)
			{
				if ((buffer[i] >> j) & 0x01)
				{
					write(CH);
					write(CL);
				}
				else
				{
					write(BCH);
					write(BCL);
				}
			}
			// vertical spacing
			write(BCH);
			write(BCL);
		}

		// horizontal spacing
		for (i = 0; i < 6; i++)
		{
			write(BCH);
			write(BCL);
		}
	}
	else
	{
		a0_h();
		for (j = 0; j < 7; j++)
		{
			for (i = 0; i < 5; i++)
			{
				// pixel group
				fillRect(X + (i * scale), Y + (j * scale), X + (i * scale) + scale - 1, Y + (j * scale) + scale - 1, ((buffer[i] >> j) & 0x01) ? color : bgcolor);
			}
			// vertical spacing
			//      ST7735_FillRect(X + (i * scale), Y + (j * scale), X + (i * scale) + scale - 1, Y + (j * scale) + scale - 1, V_SEP);
			fillRect(X + (i * scale), Y + (j * scale), X + (i * scale) + scale - 1, Y + (j * scale) + scale - 1, bgcolor);
		}
		// horizontal spacing
		//    ST7735_FillRect(X, Y + (j * scale), X + (i * scale) + scale - 1, Y + (j * scale) + scale - 1, H_SEP);
		fillRect(X, Y + (j * scale), X + (i * scale) + scale - 1, Y + (j * scale) + scale - 1, bgcolor);
	}
	cs_h();
}

void ST7745::putChar7x11(uint16_t X, uint16_t Y, uint8_t chr, uint16_t color, uint16_t bgcolor)
{
	uint16_t i, j;
	uint8_t buffer[11];
	uint8_t CH = color >> 8;
	uint8_t CL = (uint8_t)color;
	uint8_t BCH = bgcolor >> 8;
	uint8_t BCL = (uint8_t)bgcolor;

	if ((chr >= 0x20) && (chr <= 0x7F))
	{
		// ASCII[0x20-0x7F]
		memcpy(buffer, &Font7x11[(chr - 32) * 11], 11);
	}
	else if (chr >= 0xA0)
	{
		// CP1251[0xA0-0xFF]
		memcpy(buffer, &Font7x11[(chr - 64) * 11], 11);
	}
	else
	{
		// unsupported symbol
		memcpy(buffer, &Font7x11[160], 11);
	}

	cs_l();
	addrSet(X, Y, X + 7, Y + 11);
	a0_h();
	for (i = 0; i < 11; i++)
	{
		for (j = 0; j < 7; j++)
		{
			if ((buffer[i] >> j) & 0x01)
			{
				write(CH);
				write(CL);
			}
			else
			{
				write(BCH);
				write(BCL);
			}
		}
		// vertical spacing
		write(BCH);
		write(BCL);
	}

	// horizontal spacing
	for (i = 0; i < 8; i++)
	{
		write(BCH);
		write(BCL);
	}

	cs_h();
}

void ST7745::putStr7x11(uint8_t X, uint8_t Y, char* str, uint16_t color, uint16_t bgcolor)
{
	while (*str)
	{
		putChar7x11(X, Y, *str++, color, bgcolor);
		if (X < scr_width - 8) { X += 8; }
		else if (Y < scr_height - 12) { X = 0; Y += 12; }
		else { X = 0; Y = 0; }
	}
}

void ST7745::putStr5x7(uint8_t scale, uint8_t X, uint8_t Y, char* str, uint16_t color, uint16_t bgcolor)
{
	// scale equals 1 drawing faster
	if (scale == 1)
	{
		while (*str)
		{
			putChar5x7(scale, X, Y, *str++, color, bgcolor);
			if (X < scr_width - 6) { X += 6; }
			else if (Y < scr_height - 8) { X = 0; Y += 8; }
			else { X = 0; Y = 0; }
		}
	}
	else
	{
		while (*str)
		{
			putChar5x7(scale, X, Y, *str++, color, bgcolor);
			if (X < scr_width - (scale * 5) + scale) { X += (scale * 5) + scale; }
			else if (Y < scr_height - (scale * 7) + scale) { X = 0; Y += (scale * 7) + scale; }
			else { X = 0; Y = 0; }
		}
	}
}


void ST7735_DrawIcon(uint8_t X, uint8_t Y, icons::icon_16x16c2)
{
	
}

void ST7735_DrawIcon(uint8_t X, uint8_t Y, icons::icon_16x16c4)
{
	
}

void ST7735_DrawIcon(uint8_t X, uint8_t Y, icons::icon_32x32c2)
{
	
}

void ST7735_DrawIcon(uint8_t X, uint8_t Y, icons::icon_32x32c4)
{
	
}