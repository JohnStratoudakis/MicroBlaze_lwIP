# MicroBlaze_lwIP
LabVIEW FPGA + MicroBlaze + lwIP

 == Clone this repository and the submodule
git clone --recurse-submodules git@github.com:JohnStratoudakis/MicroBlaze_lwIP.git MicroBlaze_lwIP

git submodule init
git submodule update

== After project export
 - Create Block Design "d_microblaze"
 - Run Block Automation
  - Local Memory: 128 KB
  - Cache Configuration: 64 KB
  - Debug Module: None
 * Clocking Wizard:
  - Single Ended (from differential)
  - Uncheck reset
  - Uncheck locked
  - (Will by greyed out, should be Active High)
 * Processor System Reset
  - right-click on ext_reset_in and 'make external'
   - name this port reset_rtl
   - make sure it is active low (from properties)
 * 

 - From Address Editor, increase memory to 1MB for:
  * microblaze_0_local_memory/dlmb_bram_if_cntlr
  * microblaze_0_local_memory/ilmb_bram_if_cntlr
 - Run Validate Design
 - Run Generate Output Products
 - In file d_microblaze_wrapper.vhd, uncomment to bring usage of d_microblaze component
 C:/work/git/FPGANow/MicroBlaze_lwIP/labview_fpga_nic/ProjectExportForVivado/fpga_nic/User/UserRTL_d_microblaze_wrapper.vhd
 - Export Hardware
  - to the C:/work/git/FPGANow/MicroBlaze_lwIP/workspace directory
 - Launch Xilinx SDK and create bitstream
 
 == Xilinx SDK instructions ==
 - Select workspace location to C:/work/git/FPGANow/MicroBlaze_lwIP/workspace directory
 - Create Xilinx Hardware Platform Specification using the hdf file
 - Create Board Support Package
 - Import "Existing Project" C:\work\git\FPGANow\MicroBlaze_lwIP\xilinx_mb_lwip\mb_lwip
  - Do not need to copy sources in.
 
 