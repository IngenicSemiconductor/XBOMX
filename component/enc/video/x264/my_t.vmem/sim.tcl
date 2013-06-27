database -open waves -into waves.shm -default ; 
probe -create -database waves -shm -all -variable -depth all -memories -name my_probe;
probe -disable my_probe;
stop -object test.wave_en -execute {if {#test.wave_en==1} {probe -enable my_probe} else {probe -disable my_probe}} -continue
run; 
#exit;
