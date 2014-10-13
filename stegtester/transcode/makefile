IMAGE=225

test0:
	wedge -message "0123456789" $(IMAGE).jpg $(IMAGE)_jel.jpg
	unwedge $(IMAGE)_jel.jpg text.txt
	cat text.txt

test1:
	wedge -message "0123456789" $(IMAGE).jpg $(IMAGE)_jel.jpg
	python jpeg.py $(IMAGE)_jel.jpg $(IMAGE)_jel_pi.jpg 30
	unwedge $(IMAGE)_jel_pi.jpg text.txt
	cat text.txt

test2:
	python jpeg.py $(IMAGE).jpg $(IMAGE)_pi.jpg 30
	wedge -message "0123456789" $(IMAGE)_pi.jpg $(IMAGE)_pi_jel.jpg
	python jpeg.py $(IMAGE)_pi_jel.jpg $(IMAGE)_jel_pi2.jpg 30
	unwedge $(IMAGE)_jel_pi2.jpg text.txt
	cat text.txt

clean:
	rm -f *.txt
	rm -f $(IMAGE)_*

