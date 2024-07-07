if (TARGET tinyusb_device)

	add_compile_options(-Wall
	        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
	        -Wno-unused-function # we have some for the docs that aren't called
	        -Wno-maybe-uninitialized
	        )

    add_executable(PicoBrg
            main.c
            Cmd.c
            Flash.c
            Common.c
            Gpio.c
            Uart.c
            Timer.c
            TcpServer.c
            Frame.c
            )

	target_include_directories(PicoBrg PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        )

    # pull in common dependencies
    target_link_libraries(PicoBrg 
    		pico_cyw43_arch_lwip_poll
			pico_stdlib 
			hardware_adc 
			hardware_spi 
			hardware_dma 
			hardware_i2c 
			pico_i2c_slave 
			hardware_pwm 
			pico_multicore
			hardware_sync
           	pico_unique_id
          	hardware_clocks
            hardware_flash
            hardware_exception
			)

    # enable usb output, disable uart output
    pico_enable_stdio_usb(PicoBrg 1)
    pico_enable_stdio_uart(PicoBrg 0)

    # create map/bin/hex/uf2 file etc.
    pico_add_extra_outputs(PicoBrg)

elseif(PICO_ON_DEVICE)
    message(WARNING "not building PicoBrg because TinyUSB submodule is not initialized in the SDK")
endif()
