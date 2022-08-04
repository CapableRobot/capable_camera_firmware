/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright 2022 Chris Niessl for Hellbender, Inc.
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 * 
 * spi.h - Header for SPI Bus wrapper
 */

class IMU_SPI_Wrapper
{

  const uint32_t SPI_DEFAULT_CHUNK_SIZE = 4096;

  public:
  
    IMU_SPI_Wrapper();
    ~IMU_SPI_Wrapper();


    int32_t transfer(uint8_t *in_buffer, uint32_t size);
    int32_t write(uint8_t *out_buffer, uint32_t len);    
    int32_t readByte(uint8_t *in_buffer);
    int32_t readInto(uint8_t *in_buffer, uint32_t size);

  protected:

    uint32_t default_speed;
    int      current_fd;
    
    void generate_spi_ioc(struct spi_ioc_transfer &spi_ioc_to_use,
                          uint64_t tx_buf,
                          uint64_t rx_buf,
                          uint32_t len,
                          uint32_t speed_hz);

}
