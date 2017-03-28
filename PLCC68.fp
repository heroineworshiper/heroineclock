	    # was 15
	     # was 50
Element(0x00 "Plastic leadless chip carrier" "" "PLCC44" 100 325 0 100 0x00)
(
	# top row
	Pad(875 -60 875 40 20 "40" 0x100) 
	Pad(825 -60 825 40 20 "41" 0x100) 
	Pad(775 -60 775 40 20 "42" 0x100) 
	Pad(725 -60 725 40 20 "43" 0x100) 
	Pad(675 -60 675 40 20 "44" 0x100) 
	Pad(625 -60 625 40 20 "1" 0x00) 
	Pad(575 -60 575 40 20 "40" 0x100) 
	Pad(525 -60 525 40 20 "41" 0x100) 
	Pad(475 -60 475 40 20 "42" 0x100) 
	Pad(425 -60 425 40 20 "43" 0x100) 
	Pad(375 -60 375 40 20 "44" 0x100) 
	Pad(325 -60 325 40 20 "1" 0x00) 
	Pad(275 -60 275 40 20 "2" 0x100) 
	Pad(225 -60 225 40 20 "3" 0x100) 
	Pad(175 -60 175 40 20 "4" 0x100) 
	Pad(125 -60 125 40 20 "5" 0x100) 
	Pad(75 -60 75 40 20 "6" 0x100) 

	
	
	# left row
	Pad(-60 75 40 75 20 "7" 0x100) 
	Pad(-60 125 40 125 20 "8" 0x100) 
	Pad(-60 175 40 175 20 "9" 0x100) 
	Pad(-60 225 40 225 20 "10" 0x100) 
	Pad(-60 275 40 275 20 "11" 0x100) 
	Pad(-60 325 40 325 20 "12" 0x100) 
	Pad(-60 375 40 375 20 "13" 0x100) 
	Pad(-60 425 40 425 20 "14" 0x100) 
	Pad(-60 475 40 475 20 "15" 0x100) 
	Pad(-60 525 40 525 20 "16" 0x100) 
	Pad(-60 575 40 575 20 "17" 0x100) 
	Pad(-60 625 40 625 20 "12" 0x100) 
	Pad(-60 675 40 675 20 "13" 0x100) 
	Pad(-60 725 40 725 20 "14" 0x100) 
	Pad(-60 775 40 775 20 "15" 0x100) 
	Pad(-60 825 40 825 20 "16" 0x100) 
	Pad(-60 875 40 875 20 "17" 0x100) 
	
	
	
	# bottom row
	Pad(75 1010 75 910 20 "18" 0x100) 
	Pad(125 1010 125 910 20 "19" 0x100) 
	Pad(175 1010 175 910 20 "20" 0x100) 
	Pad(225 1010 225 910 20 "21" 0x100) 
	Pad(275 1010 275 910 20 "22" 0x100) 
	Pad(325 1010 325 910 20 "23" 0x100) 
	Pad(375 1010 375 910 20 "24" 0x100) 
	Pad(425 1010 425 910 20 "25" 0x100) 
	Pad(475 1010 475 910 20 "26" 0x100) 
	Pad(525 1010 525 910 20 "27" 0x100) 
	Pad(575 1010 575 910 20 "28" 0x100) 
	Pad(625 1010 625 910 20 "23" 0x100) 
	Pad(675 1010 675 910 20 "24" 0x100) 
	Pad(725 1010 725 910 20 "25" 0x100) 
	Pad(775 1010 775 910 20 "26" 0x100) 
	Pad(825 1010 825 910 20 "27" 0x100) 
	Pad(875 1010 875 910 20 "28" 0x100) 








	# right row
	Pad(1010 575 910 575 20 "29" 0x100) 
	Pad(1010 525 910 525 20 "30" 0x100) 
	Pad(1010 475 910 475 20 "31" 0x100) 
	Pad(1010 425 910 425 20 "32" 0x100) 
	Pad(1010 375 910 375 20 "33" 0x100) 
	Pad(1010 325 910 325 20 "34" 0x100) 
	Pad(1010 275 910 275 20 "35" 0x100) 
	Pad(1010 225 910 225 20 "36" 0x100) 
	Pad(1010 175 910 175 20 "37" 0x100) 
	Pad(1010 125 910 125 20 "38" 0x100) 
	Pad(1010 675 910 675 20 "33" 0x100) 
	Pad(1010 625 910 625 20 "34" 0x100) 
	Pad(1010 775 910 775 20 "35" 0x100) 
	Pad(1010 725 910 725 20 "36" 0x100) 
	Pad(1010 875 910 875 20 "37" 0x100) 
	Pad(1010 825 910 825 20 "38" 0x100) 
	Pad(1010 75 910 75 20 "39" 0x100) 











#	ElementLine(50 0 WIDTH 0 20)
#	ElementLine(WIDTH 0 WIDTH WIDTH 20)
#	ElementLine(WIDTH WIDTH 0 WIDTH 20)
#	ElementLine(0 WIDTH 0 50 20)
#	ElementLine(0 50 50 0 20)
# Modified by Thomas Olson to eliminate silkscreen blobbing over pads.
# Approach one: eliminate ElementLine transgression over pads. leave corners
# only.
#	ElementLine(600 0 650 0 10)
#	ElementLine(650 0 650 50 10)
#	ElementLine(650 600 650 650 10)
#	ElementLine(650 650 600 650 10)
#	ElementLine(50 650 0 650 10)
#	ElementLine(0 650 0 600 10)
#	ElementLine(0 50 50 0 10)
# Approach two: move outline to edge of pads.
# The outline should be 15 off. But since the pad algorithm
# is not making the square pads correctly I give it a total of 30
# to clear the pads.
# Try 40 mils, and parameterize it.  1/12/00 LRD
#	ElementLine(50 -40 690 -40 10)
#	ElementLine(690 -40 690 690 10)
#	ElementLine(690 690 -40 690 10)
#	ElementLine(-40 690 -40 50 10)
#	ElementLine(-40 50 50 -40 10)
	ElementArc(475 100 20 20 0 360 10)
	Mark(0 0)
)
