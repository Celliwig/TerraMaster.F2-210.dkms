dtbs:
	cpp -nostdinc -I include -I arch -undef -x assembler-with-cpp rtd-1296-tm-f2-210.dts rtd-1296-tm-f2-210.dtcpp
	cpp -nostdinc -I include -I arch -undef -x assembler-with-cpp rtd-1296-tm-f2-210-basic.dts rtd-1296-tm-f2-210-basic.dtcpp
	dtc -i include/ -I dts -O dtb rtd-1296-tm-f2-210.dtcpp -o rtd-1296-tm-f2-210.dtb
	dtc -i include/ -I dts -O dtb rtd-1296-tm-f2-210-basic.dtcpp -o rtd-1296-tm-f2-210-basic.dtb

clean:
	rm -f *.dtcpp *.dtb
