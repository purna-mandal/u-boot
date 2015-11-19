/*
 * (c) 2015 Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */
#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <miiphy.h>
#include <phy.h>
#include <asm/io.h>
#include <console.h>
#include <dm.h>
#include <net.h>
#include <asm/gpio.h>

#include "pic32_eth.h"

/* local definitions */
#define MAX_RX_BUF_SIZE		1536
#define MAX_RX_DESCR		PKTBUFSRX
#define MAX_TX_DESCR		2

DECLARE_GLOBAL_DATA_PTR;

struct pic32eth_device {
	struct eth_dma_desc rxd_ring[MAX_RX_DESCR];
	struct eth_dma_desc txd_ring[MAX_TX_DESCR];
	struct pic32_ectl_regs *ectl_regs;
	struct pic32_emac_regs *emac_regs;
	struct phy_device *phydev;
	phy_interface_t phyif;
	u32 phy_id; /* PHY addr */
	u32 rxd_idx; /* index of RX desc to read */
	struct gpio_desc rst_gpio;
};

__attribute__ ((weak)) void board_netphy_reset(void *dev)
{
	struct pic32eth_device *pedev = (struct pic32eth_device *)dev;

	if (!dm_gpio_is_valid(&pedev->rst_gpio))
		return;

	/* phy reset */
	dm_gpio_set_value(&pedev->rst_gpio, 0);
	udelay(300);
	dm_gpio_set_value(&pedev->rst_gpio, 1);
	udelay(300);
}

/* Initialize mii(MDIO) interface, discover which PHY is
 * attached to the device, and configure it properly.
 */
static int _mdio_init(struct pic32eth_device *pedev)
{
	struct pic32_ectl_regs *ectl_p = pedev->ectl_regs;
	struct pic32_emac_regs *emac_p = pedev->emac_regs;

	board_netphy_reset(pedev);

	/* disable RX, TX & all transactions */
	writel(ETHCON_ON | ETHCON_TXRTS | ETHCON_RXEN, &ectl_p->con1.clr);

	/* wait until not BUSY */
	while (readl(&ectl_p->stat.raw) & ETHSTAT_BUSY)
		;

	/* turn controller ON to access PHY over MII */
	writel(ETHCON_ON, &ectl_p->con1.set);

	udelay(DELAY_10MSEC);

	/* reset MAC */
	writel(EMAC_SOFTRESET, &emac_p->cfg1.set); /* reset assert */
	udelay(DELAY_10MSEC);
	writel(EMAC_SOFTRESET, &emac_p->cfg1.clr); /* reset deassert */

	/* initialize MDIO/MII */
	if (pedev->phyif == PHY_INTERFACE_MODE_RMII) {
		writel(EMAC_RMII_RESET, &emac_p->supp.set);
		udelay(DELAY_10MSEC);
		writel(EMAC_RMII_RESET, &emac_p->supp.clr);
	}

	return pic32_mdio_init(PIC32_MDIO_NAME, (ulong)&emac_p->mii);
}

static int _phy_init(struct pic32eth_device *pedev, void *dev)
{
	struct mii_dev *mii;

	mii = miiphy_get_dev_by_name(PIC32_MDIO_NAME);

	/* find & connect PHY */
	pedev->phydev = phy_connect(mii, pedev->phy_id,
				    dev, pedev->phyif);
	if (!pedev->phydev) {
		printf("%s: %s: Error, PHY connect\n", __FILE__, __func__);
		return 0;
	}

	/* Wait for phy to complete reset */
	udelay(DELAY_10MSEC);

	/* configure supported modes */
	pedev->phydev->supported = SUPPORTED_10baseT_Half |
				   SUPPORTED_10baseT_Full |
				   SUPPORTED_100baseT_Half |
				   SUPPORTED_100baseT_Full |
				   SUPPORTED_Autoneg;

	pedev->phydev->advertising = ADVERTISED_10baseT_Half |
				     ADVERTISED_10baseT_Full |
				     ADVERTISED_100baseT_Half |
				     ADVERTISED_100baseT_Full |
				     ADVERTISED_Autoneg;

	pedev->phydev->autoneg = AUTONEG_ENABLE;

	return 0;
}

/* Configure MAC based on negotiated speed and duplex
 * reported by PHY.
 */
static int _mac_adjust_link(struct pic32eth_device *pedev)
{
	struct phy_device *phydev = pedev->phydev;
	struct pic32_emac_regs *emac_p = pedev->emac_regs;

	if (!phydev->link) {
		printf("%s: No link.\n", phydev->dev->name);
		return 0;
	}

	if (phydev->duplex) {
		writel(EMAC_FULLDUP, &emac_p->cfg2.set);
		writel(FULLDUP_GAP_TIME, &emac_p->ipgt.raw);
	} else {
		writel(EMAC_FULLDUP, &emac_p->cfg2.clr);
		writel(HALFDUP_GAP_TIME, &emac_p->ipgt.raw);
	}

	switch (phydev->speed) {
	case SPEED_100:
		writel(EMAC_RMII_SPD100, &emac_p->supp.set);
		break;
	case SPEED_10:
		writel(EMAC_RMII_SPD100, &emac_p->supp.clr);
		break;
	default:
		printf("%s: Speed was bad\n", phydev->dev->name);
		return 0;
	}

	printf("pic32eth: PHY is %s with %dbase%s, %s\n",
	       phydev->drv->name,
	       phydev->speed, (phydev->port == PORT_TP) ? "T" : "X",
	       (phydev->duplex) ? "full" : "half");

	return 1;
}

static void _mac_init(struct pic32eth_device *pedev, u8 *macaddr)
{
	struct pic32_emac_regs *emac_p = pedev->emac_regs;
	u32 stat = 0, v;
	u64 expire;

	v = EMAC_TXPAUSE | EMAC_RXPAUSE | EMAC_RXENABLE;
	writel(v, &emac_p->cfg1.raw);

	v = EMAC_EXCESS | EMAC_AUTOPAD | EMAC_PADENABLE |
	    EMAC_CRCENABLE | EMAC_LENGTHCK | EMAC_FULLDUP;
	writel(v, &emac_p->cfg2.raw);

	/* recommended back-to-back inter-packet gap for 10 Mbps half duplex */
	writel(HALFDUP_GAP_TIME, &emac_p->ipgt.raw);

	/* recommended non-back-to-back interpacket gap is 0xc12 */
	writel(0xc12, &emac_p->ipgr.raw);

	/* recommended collision window retry limit is 0x370F */
	writel(0x370f, &emac_p->clrt.raw);

	/* set maximum frame length: allow VLAN tagged frame */
	writel(0x600, &emac_p->maxf.raw);

	/* set the mac address */
	writel(macaddr[0] | (macaddr[1] << 8), &emac_p->sa2.raw);
	writel(macaddr[2] | (macaddr[3] << 8), &emac_p->sa1.raw);
	writel(macaddr[4] | (macaddr[5] << 8), &emac_p->sa0.raw);

	/* default, enable 10 Mbps operation */
	writel(EMAC_RMII_SPD100, &emac_p->supp.clr);

	/* wait until link status UP or deadline elapsed */
	expire = get_ticks() + get_tbclk() * 2;
	for (; get_ticks() < expire;) {
		stat = phy_read(pedev->phydev, pedev->phy_id, MII_BMSR);
		if (stat & BMSR_LSTATUS)
			break;
	}

	if (!(stat & BMSR_LSTATUS))
		printf("MAC: Link is DOWN!\n");

	/* delay to stabilize before any tx/rx */
	udelay(DELAY_10MSEC);
}

static void _mac_reset(struct pic32eth_device *pedev)
{
	struct mii_dev *mii;
	struct pic32_emac_regs *emac_p = pedev->emac_regs;

	/* Reset MAC */
	writel(EMAC_SOFTRESET, &emac_p->cfg1.raw);
	udelay(DELAY_10MSEC);

	/* clear reset */
	writel(0, &emac_p->cfg1.raw);

	/* Reset MII */
	mii = pedev->phydev->bus;
	if (mii && mii->reset)
		mii->reset(mii);
}

/* initializes the MAC and PHY, then establishes a link */
static void _eth_ctrl_reset(struct pic32eth_device *pedev)
{
	u32 v;
	struct pic32_ectl_regs *ectl_p = pedev->ectl_regs;

	/* disable RX, TX & any other transactions */
	writel(ETHCON_ON | ETHCON_TXRTS | ETHCON_RXEN, &ectl_p->con1.clr);

	/* wait until not BUSY */
	while (readl(&ectl_p->stat.raw) & ETHSTAT_BUSY)
		;
	/* decrement received buffcnt to zero. */
	while (readl(&ectl_p->stat.raw) & ETHSTAT_BUFCNT)
		writel(ETHCON_BUFCDEC, &ectl_p->con1.set);

	/* clear any existing interrupt event */
	writel(0xffffffff, &ectl_p->irq.clr);

	/* clear RX/TX start address */
	writel(0xffffffff, &ectl_p->txst.clr);
	writel(0xffffffff, &ectl_p->rxst.clr);

	/* clear the receive filters */
	writel(0x00ff, &ectl_p->rxfc.clr);

	/* set the receive filters
	 * ETH_FILT_CRC_ERR_REJECT
	 * ETH_FILT_RUNT_REJECT
	 * ETH_FILT_UCAST_ACCEPT
	 * ETH_FILT_MCAST_ACCEPT
	 * ETH_FILT_BCAST_ACCEPT
	 */
	v = ETHRXFC_BCEN | ETHRXFC_MCEN | ETHRXFC_UCEN |
	    ETHRXFC_RUNTEN | ETHRXFC_CRCOKEN;
	writel(v, &ectl_p->rxfc.set);

	/* turn controller ON to access PHY over MII */
	writel(ETHCON_ON, &ectl_p->con1.set);
}

static void _eth_desc_init(struct pic32eth_device *pedev)
{
	u32 idx, bufsz;
	struct eth_dma_desc *rxd;
	struct pic32_ectl_regs *ectl_p = pedev->ectl_regs;

	pedev->rxd_idx = 0;
	for (idx = 0; idx < MAX_RX_DESCR; idx++) {
		rxd = &pedev->rxd_ring[idx];

		/* hw owned */
		rxd->hdr = EDH_NPV | EDH_EOWN | EDH_STICKY;

		/* packet buffer address */
		rxd->data_buff = virt_to_phys(net_rx_packets[idx]);

		/* link to next desc */
		rxd->next_ed = virt_to_phys(rxd + 1);

		/* reset status */
		rxd->stat1 = 0;
		rxd->stat2 = 0;

		/* decrement bufcnt */
		writel(ETHCON_BUFCDEC, &ectl_p->con1.set);
	}

	/* link last descr to beginning of list */
	rxd->next_ed = virt_to_phys(&pedev->rxd_ring[0]);

	/* flush rx ring */
	__dcache_flush(pedev->rxd_ring, sizeof(pedev->rxd_ring));

	/* set rx desc-ring start address */
	writel((ulong)virt_to_phys(&pedev->rxd_ring[0]), &ectl_p->rxst.raw);

	/* RX Buffer size */
	bufsz = readl(&ectl_p->con2.raw);
	bufsz &= ~(ETHCON_RXBUFSZ << ETHCON_RXBUFSZ_SHFT);
	bufsz |= ((MAX_RX_BUF_SIZE / 16) << ETHCON_RXBUFSZ_SHFT);
	writel(bufsz, &ectl_p->con2.raw);

	/* enable the receiver in hardware which allows hardware
	 * to DMA received pkts to the descriptor pointer address.
	 */
	writel(ETHCON_RXEN, &ectl_p->con1.set);
}

static void _pic32eth_halt(struct pic32eth_device *pedev)
{
	struct pic32_ectl_regs *ectl_p = pedev->ectl_regs;
	struct pic32_emac_regs *emac_p = pedev->emac_regs;

	/* Reset the phy if the controller is enabled */
	if (readl(&ectl_p->con1.raw) & ETHCON_ON)
		phy_reset(pedev->phydev);

	/* Shut down the PHY */
	phy_shutdown(pedev->phydev);

	/* Stop rx/tx */
	writel(ETHCON_TXRTS | ETHCON_RXEN, &ectl_p->con1.clr);
	udelay(DELAY_10MSEC);

	/* reset MAC */
	writel(EMAC_SOFTRESET, &emac_p->cfg1.raw);

	/* clear reset */
	writel(0, &emac_p->cfg1.raw);
	udelay(DELAY_10MSEC);

	/* disable controller */
	writel(ETHCON_ON, &ectl_p->con1.clr);
	udelay(DELAY_10MSEC);

	/* wait until everything is down */
	while (readl(&ectl_p->stat.raw) & ETHSTAT_BUSY)
		;

	/* clear any existing interrupt event */
	writel(0xffffffff, &ectl_p->irq.clr);
}

static int _pic32eth_init(struct pic32eth_device *pedev, u8 *macaddr)
{
	/* configure controller */
	_eth_ctrl_reset(pedev);

	/* reset mac_regs */
	_mac_reset(pedev);

	/* configure the PHY */
	phy_config(pedev->phydev);

	/* initialize MAC */
	_mac_init(pedev, macaddr);

	/* init RX descriptor; TX descriptor is taken care in xmit */
	_eth_desc_init(pedev);

	/* Start up & update link status of PHY */
	phy_startup(pedev->phydev);

	/* adjust mac with phy link status */
	if (!_mac_adjust_link(pedev)) {
		_pic32eth_halt(pedev);
		return -1;
	}

	/* If there's no link, fail */
	return pedev->phydev->link ? 0 : -1;
}

static int _pic32eth_xmit(struct pic32eth_device *pedev,
			  void *packet, int length)
{
	u64 deadline;
	struct eth_dma_desc *txd;
	struct pic32_ectl_regs *ectl_p = pedev->ectl_regs;

	txd = &pedev->txd_ring[0];

	/* set proper flags & length in descriptor header */
	txd->hdr = EDH_SOP | EDH_EOP | EDH_EOWN | EDH_BCOUNT(length);

	/* pass buffer address to hardware */
	txd->data_buff = virt_to_phys(packet);

	debug("%s: %d / .hdr %x, .data_buff %x, .stat %x, .nexted %x\n",
	      __func__, __LINE__, txd->hdr, txd->data_buff, txd->stat2,
	      txd->next_ed);

	/* cache flush (packet) */
	__dcache_flush(packet, length);

	/* cache flush (txd) */
	__dcache_flush(txd, sizeof(*txd));

	/* pass descriptor table base to h/w */
	writel(virt_to_phys(txd), &ectl_p->txst.raw);

	/* ready to send enabled, hardware can now send the packet(s) */
	writel(ETHCON_TXRTS | ETHCON_ON, &ectl_p->con1.set);

	/* wait until tx has completed and h/w has released ownership
	 * of the tx descriptor or timeout elapsed.
	 */
	deadline = get_ticks() + get_tbclk();
	for (;;) {
		/* check timeout */
		if (get_ticks() > deadline)
			return -ETIMEDOUT;

		/* tx completed? */
		if (readl(&ectl_p->con1.raw) & ETHCON_TXRTS)
			continue;

		/* h/w not released ownership yet? */
		__dcache_invalidate(txd, sizeof(*txd));
		if (!(txd->hdr & EDH_EOWN))
			break;

		if (ctrlc())
			break;
	}

	return 0;
}

static int _pic32eth_free_packet(struct pic32eth_device *pedev, uchar *packet)
{
	int idx;
	struct eth_dma_desc *rxd;
	struct pic32_ectl_regs *ectl_p = pedev->ectl_regs;

	idx = pedev->rxd_idx;
	if (packet != net_rx_packets[idx]) {
		printf("rxd_id %d: packet is not matched,\n", idx);
		return -EAGAIN;
	}

	/* prepare for new receive */
	rxd = &pedev->rxd_ring[idx];
	rxd->hdr = EDH_STICKY | EDH_NPV | EDH_EOWN;

	__dcache_flush(rxd, sizeof(*rxd));

	/* decrement rx pkt count */
	writel(ETHCON_BUFCDEC, &ectl_p->con1.set);

	debug("%s: %d /fill-idx %i, .hdr=%x, .data_buff %x, .stat %x, .nexted %x / rx-idx %i\n",
	      __func__, __LINE__, idx, rxd->hdr, rxd->data_buff,
	      rxd->stat2, rxd->next_ed, pedev->rxd_idx);

	pedev->rxd_idx = (pedev->rxd_idx + 1) % MAX_RX_DESCR;

	return 0;
}

static int _pic32eth_rx_poll(struct pic32eth_device *pedev, uchar **packetp)
{
	int idx, top;
	u32 rx_stat;
	int rx_count, bytes_rcvd = 0;
	struct eth_dma_desc *rxd;

	top = (pedev->rxd_idx + MAX_RX_DESCR - 1) % MAX_RX_DESCR;

	/* non-blocking receive loop - receive until nothing left to receive */
	for (idx = pedev->rxd_idx; idx != top; idx = (idx + 1) % MAX_RX_DESCR) {
		rxd = &pedev->rxd_ring[idx];

		/* check ownership */
		__dcache_invalidate(rxd, sizeof(*rxd));
		if (rxd->hdr & EDH_EOWN)
			break;

		if (rxd->hdr & EDH_SOP) {
			if (!(rxd->hdr & EDH_EOP)) {
				printf("%s: %s, rx pkt across multiple descr\n",
				       __FILE__, __func__);
				goto refill_one;
			}

			rx_stat = rxd->stat2;
			rx_count = RSV_RX_COUNT(rx_stat);

			debug("%s: %d /rx-idx %i, .hdr=%x, .data_buff %x, .stat %x, .nexted %x\n",
			      __func__, __LINE__, idx, rxd->hdr,
			      rxd->data_buff, rxd->stat2, rxd->next_ed);

			/* we can do some basic checks */
			if ((!RSV_RX_OK(rx_stat)) || RSV_CRC_ERR(rx_stat)) {
				debug("%s: %s: Error, rx problem detected\n",
				      __FILE__, __func__);
				goto refill_one;
			}

			/* invalidate dcache */
			__dcache_invalidate(net_rx_packets[idx], rx_count);

			/* increment number of bytes rcvd (ignore CRC) */
			bytes_rcvd += (rx_count - 4);

			/* Pass the packet to protocol layer */
			*packetp = net_rx_packets[idx];
			break;
refill_one:
			_pic32eth_free_packet(pedev, net_rx_packets[idx]);
		}

		if (ctrlc())
			break;
	}

	return bytes_rcvd ? bytes_rcvd : -EAGAIN;
}

static int pic32_eth_start(struct udevice *dev)
{
	struct eth_pdata *pdata = dev_get_platdata(dev);
	struct pic32eth_device *priv = dev_get_priv(dev);

	return _pic32eth_init(priv, pdata->enetaddr);
}

static int pic32_eth_send(struct udevice *dev, void *packet, int length)
{
	struct pic32eth_device *priv = dev_get_priv(dev);

	return _pic32eth_xmit(priv, packet, length);
}

static int pic32_eth_recv(struct udevice *dev, int flags, uchar **packetp)
{
	struct pic32eth_device *priv = dev_get_priv(dev);

	return _pic32eth_rx_poll(priv, packetp);
}

static int pic32_eth_free_pkt(struct udevice *dev, uchar *packet, int length)
{
	struct pic32eth_device *priv = dev_get_priv(dev);

	return _pic32eth_free_packet(priv, packet);
}

static void pic32_eth_stop(struct udevice *dev)
{
	struct pic32eth_device *priv = dev_get_priv(dev);

	return _pic32eth_halt(priv);
}

static const struct eth_ops pic32_eth_ops = {
	.start			= pic32_eth_start,
	.send			= pic32_eth_send,
	.recv			= pic32_eth_recv,
	.free_pkt		= pic32_eth_free_pkt,
	.stop			= pic32_eth_stop,
};

static int pic32_eth_probe(struct udevice *dev)
{
	struct eth_pdata *pdata = dev_get_platdata(dev);
	struct pic32eth_device *priv = dev_get_priv(dev);
	u32 iobase = pdata->iobase;
	int phy_id = 0;

#if defined(CONFIG_PHY_ADDR)
	phy_id = CONFIG_PHY_ADDR;
#endif
	/* initialize */
	priv->phy_id	= phy_id;
	priv->phyif	= pdata->phy_interface;
	priv->ectl_regs	= (struct pic32_ectl_regs *)(iobase);
	priv->emac_regs	= (struct pic32_emac_regs *)(iobase + PIC32_EMAC1CFG1);

	gpio_request_by_name_nodev(gd->fdt_blob, dev->of_offset,
				   "reset-gpios", 0,
				   &priv->rst_gpio, GPIOD_IS_OUT);
	_mdio_init(priv);

	return _phy_init(priv, dev);
}

static int pic32_eth_remove(struct udevice *dev)
{
	struct pic32eth_device *priv = dev_get_priv(dev);
	struct mii_dev *bus;

	dm_gpio_free(dev, &priv->rst_gpio);
	phy_shutdown(priv->phydev);
	free(priv->phydev);
	bus = miiphy_get_dev_by_name(PIC32_MDIO_NAME);
	mdio_unregister(bus);
	mdio_free(bus);

	return 0;
}

static int pic32_eth_ofdata_to_platdata(struct udevice *dev)
{
	struct eth_pdata *pdata = dev_get_platdata(dev);
	const char *phy_mode;

	pdata->iobase = dev_get_addr(dev);
	pdata->phy_interface = -1;
	phy_mode = fdt_getprop(gd->fdt_blob, dev->of_offset, "phy-mode", NULL);
	if (phy_mode)
		pdata->phy_interface = phy_get_interface_by_name(phy_mode);

	if (pdata->phy_interface == -1) {
		debug("%s: Invalid PHY interface '%s'\n", __func__, phy_mode);
		return -EINVAL;
	}

	return 0;
}

static const struct udevice_id pic32_eth_ids[] = {
	{ .compatible = "microchip,pic32mzda-eth" },
	{ }
};

U_BOOT_DRIVER(pic32_ethernet) = {
	.name			= "pic32_ethernet",
	.id			= UCLASS_ETH,
	.of_match		= pic32_eth_ids,
	.ofdata_to_platdata	= pic32_eth_ofdata_to_platdata,
	.probe			= pic32_eth_probe,
	.remove			= pic32_eth_remove,
	.ops			= &pic32_eth_ops,
	.priv_auto_alloc_size	= sizeof(struct pic32eth_device),
	.platdata_auto_alloc_size	= sizeof(struct eth_pdata),
};
