INSTALLER_VER_MAJOR = 1
INSTALLER_VER_MINOR = 2

TOOLS = $(CURDIR)/../../pywii/pywii-tools
CERTS = $(CURDIR)/dpki/certs
DPKI_ISSUER_TIK = Root-CA00000002-XS00000006
DPKI_ISSUER_TMD = Root-CA00000002-CP00000007

all: channel_retail.wad

dpki: channel_dpki.wad

channel_retail.wad: retail/cetk retail/tmd 00000000 00000001
	@cp -f 00000000 retail
	@cp -f 00000001 retail
	@cp -f footer retail
	@python2 $(TOOLS)/wadpack.py $@ retail/

channel_dpki.wad: dpki/cetk dpki/tmd 00000000 00000001
	@cp -f 00000000 retail
	@cp -f 00000001 retail
	@cp -f footer retail
	@python2 $(TOOLS)/wadpack.py -dpki $@ dpki/

00000000: ../banner/channel.imet
	@cat footer $< > $@

00000001: ../channelapp/channelapp-channel.dol
	@cp $< $@

retail/cetk: cetk.template
	@cp $< $@
	@python2 $(TOOLS)/tikfix.py $@

dpki/cetk: retail/cetk
	@python2 $(TOOLS)/dpkisign.py -cetk $< $@ $(CERTS) $(DPKI_ISSUER_TIK)

retail/tmd: tmd.template 00000000 00000001
	@cp $< $@
	@python2 $(TOOLS)/tmdupdatecr.py $@ $(CURDIR)
	@python2 $(TOOLS)/tmdvers.py $@ $(INSTALLER_VER_MAJOR) $(INSTALLER_VER_MINOR)

dpki/tmd: retail/tmd
	@python2 $(TOOLS)/dpkisign.py -tmd $< $@ $(CERTS) $(DPKI_ISSUER_TMD)

check: all dpki
	@echo ===== RETAIL =====
	@python2 $(TOOLS)/tikinfo.py retail/cetk retail/certs
	@python2 $(TOOLS)/tmdinfo.py retail/tmd retail/certs
	@echo ===== DPKI =====
	@python2 $(TOOLS)/tikinfo.py -dpki dpki/cetk dpki/certs
	@python2 $(TOOLS)/tmdinfo.py -dpki dpki/tmd dpki/certs

clean:
	rm -f retail/cetk dpki/cetk retail/tmd dpki/tmd retail/footer dpki/footer 00000000 00000001 \
	retail/00000000 retail/00000001 dpki/00000000 dpki/00000001 channel_retail.wad channel_dpki.wad

