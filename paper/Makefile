all: clean Ultra_low_bandwidth_video_surveillance_system.pdf

pdf: Ultra_low_bandwidth_video_surveillance_system.pdf

Ultra_low_bandwidth_video_surveillance_system.pdf: Ultra_low_bandwidth_video_surveillance_system.tex
	pdflatex Ultra_low_bandwidth_video_surveillance_system
	bibtex Ultra_low_bandwidth_video_surveillance_system
	pdflatex Ultra_low_bandwidth_video_surveillance_system.tex
	pdflatex Ultra_low_bandwidth_video_surveillance_system.tex
clean:
	rm -f *.ps *.dvi *.aux *.toc *.idx *.ind *.ilg *.log *.out *.pdf *.blg *.bbl
