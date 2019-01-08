# MicroBlaze_lwIP
LabVIEW FPGA + MicroBlaze + lwIP

 ## Clone this repository and the submodule
git clone --recurse-submodules git@github.com:JohnStratoudakis/MicroBlaze_lwIP.git MicroBlaze_lwIP

git submodule init
git submodule update

* Open the project using LabVIEW 2018 32-bit
 * labview_fpga_nic\labview_fpga_nic.lvproj
* Right-click the build specification and select "Build"
 * FPGA\Build Specifications\fpga_nic
* After project export completes, it will give you the option to "Launch Vivado"
* Open the TCL Console Window
* Change directory to the xilinx_mb_lwip directory
 * Z:/work/git/fpganow/MicroBlaze_lwIP/xilinx_mb_lwip
* Source the microblaze design tcl script
 * source d_microblaze.tcl
* Uncomment everything in the UserRTL_d_microblaze_wrapper.vhd file:
 * Design sources
  * PXIe6592R_Top_Gen2x8
   * PXIe6592RWindow
    * theCLIPS
     * UserRTL_microblaze_CLIP1
* Open the design by double-clicking on the child node named "d_microblaze_i"
* Click on the Address Editor Window, increase memory to 1MB for:
  * microblaze_0_local_memory/dlmb_bram_if_cntlr
  * microblaze_0_local_memory/ilmb_bram_if_cntlr
* click the save icon
* Click "Run Synthesis"
* file export hardware
* start xilinx sdk

== Old (Manual) Design generation instructions
* Create Block Design "d_microblaze"
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
 - Increase heap and stack space to 8kb.
 
 Z:\work\git\fpganow\MicroBlaze_lwIP\labview_fpga_nic\ProjectExportForVivado\fpga_nic\VivadoProject\fpga_nic.sdk\PXIe6592R_Top_Gen2x8.hdf
