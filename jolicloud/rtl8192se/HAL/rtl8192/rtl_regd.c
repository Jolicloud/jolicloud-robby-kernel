/*
 * Copyright (c) 2008-2009 Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef CONFIG_CRDA

#include <linux/kernel.h>
#include <linux/slab.h>
#include <net/cfg80211.h>
#include "rtl_core.h"

/* Regpair to CTL band mapping */
static struct reg_dmn_pair_mapping regDomainPairs[] = {
        /* regpair, 5 GHz CTL, 2 GHz CTL */
        {NO_ENUMRD, DEBUG_REG_DMN, DEBUG_REG_DMN},
        {NULL1_WORLD, NO_CTL, CTL_ETSI},
        {NULL1_ETSIB, NO_CTL, CTL_ETSI},
        {NULL1_ETSIC, NO_CTL, CTL_ETSI},

        {FCC2_FCCA, CTL_FCC, CTL_FCC},
        {FCC2_WORLD, CTL_FCC, CTL_ETSI},
        {FCC2_ETSIC, CTL_FCC, CTL_ETSI},
        {FCC3_FCCA, CTL_FCC, CTL_FCC},
        {FCC3_WORLD, CTL_FCC, CTL_ETSI},
        {FCC4_FCCA, CTL_FCC, CTL_FCC},
        {FCC5_FCCA, CTL_FCC, CTL_FCC},
        {FCC6_FCCA, CTL_FCC, CTL_FCC},
        {FCC6_WORLD, CTL_FCC, CTL_ETSI},

        {ETSI1_WORLD, CTL_ETSI, CTL_ETSI},
        {ETSI2_WORLD, CTL_ETSI, CTL_ETSI},
        {ETSI3_WORLD, CTL_ETSI, CTL_ETSI},
        {ETSI4_WORLD, CTL_ETSI, CTL_ETSI},
        {ETSI5_WORLD, CTL_ETSI, CTL_ETSI},
        {ETSI6_WORLD, CTL_ETSI, CTL_ETSI},

        /* XXX: For ETSI3_ETSIA, Was NO_CTL meant for the 2 GHz band ? */
        {ETSI3_ETSIA, CTL_ETSI, CTL_ETSI},
        {FRANCE_RES, CTL_ETSI, CTL_ETSI},

        {FCC1_WORLD, CTL_FCC, CTL_ETSI},
        {FCC1_FCCA, CTL_FCC, CTL_FCC},
        {APL1_WORLD, CTL_FCC, CTL_ETSI},
        {APL2_WORLD, CTL_FCC, CTL_ETSI},
        {APL3_WORLD, CTL_FCC, CTL_ETSI},
        {APL4_WORLD, CTL_FCC, CTL_ETSI},
        {APL5_WORLD, CTL_FCC, CTL_ETSI},
        {APL6_WORLD, CTL_ETSI, CTL_ETSI},
        {APL8_WORLD, CTL_ETSI, CTL_ETSI},
        {APL9_WORLD, CTL_ETSI, CTL_ETSI},

        {APL3_FCCA, CTL_FCC, CTL_FCC},
        {APL1_ETSIC, CTL_FCC, CTL_ETSI},
        {APL2_ETSIC, CTL_FCC, CTL_ETSI},
        {APL2_APLD, CTL_FCC, NO_CTL},

        {MKK1_MKKA, CTL_MKK, CTL_MKK},
        {MKK1_MKKB, CTL_MKK, CTL_MKK},
        {MKK1_FCCA, CTL_MKK, CTL_FCC},
        {MKK1_MKKA1, CTL_MKK, CTL_MKK},
        {MKK1_MKKA2, CTL_MKK, CTL_MKK},
        {MKK1_MKKC, CTL_MKK, CTL_MKK},

        {MKK2_MKKA, CTL_MKK, CTL_MKK},
        {MKK3_MKKA, CTL_MKK, CTL_MKK},
        {MKK3_MKKB, CTL_MKK, CTL_MKK},
        {MKK3_MKKA1, CTL_MKK, CTL_MKK},
        {MKK3_MKKA2, CTL_MKK, CTL_MKK},
        {MKK3_MKKC, CTL_MKK, CTL_MKK},
        {MKK3_FCCA, CTL_MKK, CTL_FCC},

        {MKK4_MKKA, CTL_MKK, CTL_MKK},
        {MKK4_MKKB, CTL_MKK, CTL_MKK},
        {MKK4_MKKA1, CTL_MKK, CTL_MKK},
        {MKK4_MKKA2, CTL_MKK, CTL_MKK},
        {MKK4_MKKC, CTL_MKK, CTL_MKK},
        {MKK4_FCCA, CTL_MKK, CTL_FCC},

        {MKK5_MKKB, CTL_MKK, CTL_MKK},
        {MKK5_MKKA2, CTL_MKK, CTL_MKK},
        {MKK5_MKKC, CTL_MKK, CTL_MKK},

        {MKK6_MKKB, CTL_MKK, CTL_MKK},
        {MKK6_MKKA1, CTL_MKK, CTL_MKK},
        {MKK6_MKKA2, CTL_MKK, CTL_MKK},
        {MKK6_MKKC, CTL_MKK, CTL_MKK},
        {MKK6_FCCA, CTL_MKK, CTL_FCC},

        {MKK7_MKKB, CTL_MKK, CTL_MKK},
        {MKK7_MKKA1, CTL_MKK, CTL_MKK},
        {MKK7_MKKA2, CTL_MKK, CTL_MKK},
        {MKK7_MKKC, CTL_MKK, CTL_MKK},
        {MKK7_FCCA, CTL_MKK, CTL_FCC},

        {MKK8_MKKB, CTL_MKK, CTL_MKK},
        {MKK8_MKKA2, CTL_MKK, CTL_MKK},
        {MKK8_MKKC, CTL_MKK, CTL_MKK},
       {MKK9_MKKA, CTL_MKK, CTL_MKK},
        {MKK9_FCCA, CTL_MKK, CTL_FCC},
        {MKK9_MKKA1, CTL_MKK, CTL_MKK},
        {MKK9_MKKA2, CTL_MKK, CTL_MKK},
        {MKK9_MKKC, CTL_MKK, CTL_MKK},

        {MKK10_MKKA, CTL_MKK, CTL_MKK},
        {MKK10_FCCA, CTL_MKK, CTL_FCC},
        {MKK10_MKKA1, CTL_MKK, CTL_MKK},
        {MKK10_MKKA2, CTL_MKK, CTL_MKK},
        {MKK10_MKKC, CTL_MKK, CTL_MKK},

        {MKK11_MKKA, CTL_MKK, CTL_MKK},
        {MKK11_FCCA, CTL_MKK, CTL_FCC},
        {MKK11_MKKA1, CTL_MKK, CTL_MKK},
        {MKK11_MKKA2, CTL_MKK, CTL_MKK},
        {MKK11_MKKC, CTL_MKK, CTL_MKK},

        {MKK12_MKKA, CTL_MKK, CTL_MKK},
        {MKK12_FCCA, CTL_MKK, CTL_FCC},
        {MKK12_MKKA1, CTL_MKK, CTL_MKK},
        {MKK12_MKKA2, CTL_MKK, CTL_MKK},
        {MKK12_MKKC, CTL_MKK, CTL_MKK},

        {MKK13_MKKB, CTL_MKK, CTL_MKK},
        {MKK14_MKKA1, CTL_MKK, CTL_MKK},
        {MKK15_MKKA1, CTL_MKK, CTL_MKK},

        {WOR0_WORLD, NO_CTL, NO_CTL},
        {WOR1_WORLD, NO_CTL, NO_CTL},
        {WOR2_WORLD, NO_CTL, NO_CTL},
        {WOR3_WORLD, NO_CTL, NO_CTL},
        {WOR4_WORLD, NO_CTL, NO_CTL},
        {WOR5_ETSIC, NO_CTL, NO_CTL},
        {WOR01_WORLD, NO_CTL, NO_CTL},
        {WOR02_WORLD, NO_CTL, NO_CTL},
        {EU1_WORLD, NO_CTL, NO_CTL},
        {WOR9_WORLD, NO_CTL, NO_CTL},
        {WORA_WORLD, NO_CTL, NO_CTL},
        {WORB_WORLD, NO_CTL, NO_CTL},
};

static struct country_code_to_enum_rd allCountries[] = {
        {CTRY_DEBUG, NO_ENUMRD, "DB"},
        {CTRY_DEFAULT, FCC1_FCCA, "CO"},
        {CTRY_ALBANIA, NULL1_WORLD, "AL"},
        {CTRY_ALGERIA, NULL1_WORLD, "DZ"},
        {CTRY_ARGENTINA, APL3_WORLD, "AR"},
        {CTRY_ARMENIA, ETSI4_WORLD, "AM"},
        {CTRY_AUSTRALIA, FCC2_WORLD, "AU"},
        {CTRY_AUSTRALIA2, FCC6_WORLD, "AU"},
        {CTRY_AUSTRIA, ETSI1_WORLD, "AT"},
        {CTRY_AZERBAIJAN, ETSI4_WORLD, "AZ"},
        {CTRY_BAHRAIN, APL6_WORLD, "BH"},
        {CTRY_BELARUS, ETSI1_WORLD, "BY"},
        {CTRY_BELGIUM, ETSI1_WORLD, "BE"},
        {CTRY_BELGIUM2, ETSI4_WORLD, "BL"},
        {CTRY_BELIZE, APL1_ETSIC, "BZ"},
        {CTRY_BOLIVIA, APL1_ETSIC, "BO"},
        {CTRY_BOSNIA_HERZ, ETSI1_WORLD, "BA"},
        {CTRY_BRAZIL, FCC3_WORLD, "BR"},
        {CTRY_BRUNEI_DARUSSALAM, APL1_WORLD, "BN"},
        {CTRY_BULGARIA, ETSI6_WORLD, "BG"},
        {CTRY_CANADA, FCC2_FCCA, "CA"},
        {CTRY_CANADA2, FCC6_FCCA, "CA"},
        {CTRY_CHILE, APL6_WORLD, "CL"},
        {CTRY_CHINA, APL1_WORLD, "CN"},
        {CTRY_COLOMBIA, FCC1_FCCA, "CO"},
        {CTRY_COSTA_RICA, FCC1_WORLD, "CR"},
        {CTRY_CROATIA, ETSI3_WORLD, "HR"},
        {CTRY_CYPRUS, ETSI1_WORLD, "CY"},
        {CTRY_CZECH, ETSI3_WORLD, "CZ"},
        {CTRY_DENMARK, ETSI1_WORLD, "DK"},
        {CTRY_DOMINICAN_REPUBLIC, FCC1_FCCA, "DO"},
        {CTRY_ECUADOR, FCC1_WORLD, "EC"},
        {CTRY_EGYPT, ETSI3_WORLD, "EG"},
        {CTRY_EL_SALVADOR, FCC1_WORLD, "SV"},
        {CTRY_ESTONIA, ETSI1_WORLD, "EE"},
        {CTRY_FINLAND, ETSI1_WORLD, "FI"},
        {CTRY_FRANCE, ETSI1_WORLD, "FR"},
        {CTRY_GEORGIA, ETSI4_WORLD, "GE"},
        {CTRY_GERMANY, ETSI1_WORLD, "DE"},
        {CTRY_GREECE, ETSI1_WORLD, "GR"},
        {CTRY_GUATEMALA, FCC1_FCCA, "GT"},
        {CTRY_HONDURAS, NULL1_WORLD, "HN"},
        {CTRY_HONG_KONG, FCC2_WORLD, "HK"},
        {CTRY_HUNGARY, ETSI1_WORLD, "HU"},
        {CTRY_ICELAND, ETSI1_WORLD, "IS"},
        {CTRY_INDIA, APL6_WORLD, "IN"},
        {CTRY_INDONESIA, APL1_WORLD, "ID"},
        {CTRY_IRAN, APL1_WORLD, "IR"},
        {CTRY_IRELAND, ETSI1_WORLD, "IE"},
        {CTRY_ISRAEL, NULL1_WORLD, "IL"},
        {CTRY_ITALY, ETSI1_WORLD, "IT"},
        {CTRY_JAMAICA, ETSI1_WORLD, "JM"},

        {CTRY_JAPAN, MKK1_MKKA, "JP"},
        {CTRY_JAPAN1, MKK1_MKKB, "JP"},
        {CTRY_JAPAN2, MKK1_FCCA, "JP"},
        {CTRY_JAPAN3, MKK2_MKKA, "JP"},
        {CTRY_JAPAN4, MKK1_MKKA1, "JP"},
        {CTRY_JAPAN5, MKK1_MKKA2, "JP"},
        {CTRY_JAPAN6, MKK1_MKKC, "JP"},
        {CTRY_JAPAN7, MKK3_MKKB, "JP"},
        {CTRY_JAPAN8, MKK3_MKKA2, "JP"},
        {CTRY_JAPAN9, MKK3_MKKC, "JP"},
        {CTRY_JAPAN10, MKK4_MKKB, "JP"},
        {CTRY_JAPAN11, MKK4_MKKA2, "JP"},
        {CTRY_JAPAN12, MKK4_MKKC, "JP"},
        {CTRY_JAPAN13, MKK5_MKKB, "JP"},
        {CTRY_JAPAN14, MKK5_MKKA2, "JP"},
        {CTRY_JAPAN15, MKK5_MKKC, "JP"},
        {CTRY_JAPAN16, MKK6_MKKB, "JP"},
        {CTRY_JAPAN17, MKK6_MKKA2, "JP"},
        {CTRY_JAPAN18, MKK6_MKKC, "JP"},
        {CTRY_JAPAN19, MKK7_MKKB, "JP"},
        {CTRY_JAPAN20, MKK7_MKKA2, "JP"},
        {CTRY_JAPAN21, MKK7_MKKC, "JP"},
        {CTRY_JAPAN22, MKK8_MKKB, "JP"},
        {CTRY_JAPAN23, MKK8_MKKA2, "JP"},
        {CTRY_JAPAN24, MKK8_MKKC, "JP"},
        {CTRY_JAPAN25, MKK3_MKKA, "JP"},
        {CTRY_JAPAN26, MKK3_MKKA1, "JP"},
        {CTRY_JAPAN27, MKK3_FCCA, "JP"},
        {CTRY_JAPAN28, MKK4_MKKA1, "JP"},
        {CTRY_JAPAN29, MKK4_FCCA, "JP"},
        {CTRY_JAPAN30, MKK6_MKKA1, "JP"},
        {CTRY_JAPAN31, MKK6_FCCA, "JP"},
        {CTRY_JAPAN32, MKK7_MKKA1, "JP"},
        {CTRY_JAPAN33, MKK7_FCCA, "JP"},
        {CTRY_JAPAN34, MKK9_MKKA, "JP"},
        {CTRY_JAPAN35, MKK10_MKKA, "JP"},
        {CTRY_JAPAN36, MKK4_MKKA, "JP"},
        {CTRY_JAPAN37, MKK9_FCCA, "JP"},
        {CTRY_JAPAN38, MKK9_MKKA1, "JP"},
        {CTRY_JAPAN39, MKK9_MKKC, "JP"},
       {CTRY_JAPAN40, MKK9_MKKA2, "JP"},
        {CTRY_JAPAN41, MKK10_FCCA, "JP"},
        {CTRY_JAPAN42, MKK10_MKKA1, "JP"},
        {CTRY_JAPAN43, MKK10_MKKC, "JP"},
        {CTRY_JAPAN44, MKK10_MKKA2, "JP"},
        {CTRY_JAPAN45, MKK11_MKKA, "JP"},
        {CTRY_JAPAN46, MKK11_FCCA, "JP"},
        {CTRY_JAPAN47, MKK11_MKKA1, "JP"},
        {CTRY_JAPAN48, MKK11_MKKC, "JP"},
        {CTRY_JAPAN49, MKK11_MKKA2, "JP"},
        {CTRY_JAPAN50, MKK12_MKKA, "JP"},
        {CTRY_JAPAN51, MKK12_FCCA, "JP"},
        {CTRY_JAPAN52, MKK12_MKKA1, "JP"},
        {CTRY_JAPAN53, MKK12_MKKC, "JP"},
        {CTRY_JAPAN54, MKK12_MKKA2, "JP"},
        {CTRY_JAPAN57, MKK13_MKKB, "JP"},
        {CTRY_JAPAN58, MKK14_MKKA1, "JP"},
        {CTRY_JAPAN59, MKK15_MKKA1, "JP"},

        {CTRY_JORDAN, ETSI2_WORLD, "JO"},
        {CTRY_KAZAKHSTAN, NULL1_WORLD, "KZ"},
        {CTRY_KOREA_NORTH, APL9_WORLD, "KP"},
        {CTRY_KOREA_ROC, APL9_WORLD, "KR"},
        {CTRY_KOREA_ROC2, APL2_WORLD, "K2"},
        {CTRY_KOREA_ROC3, APL9_WORLD, "K3"},
        {CTRY_KUWAIT, NULL1_WORLD, "KW"},
        {CTRY_LATVIA, ETSI1_WORLD, "LV"},
        {CTRY_LEBANON, NULL1_WORLD, "LB"},
        {CTRY_LIECHTENSTEIN, ETSI1_WORLD, "LI"},
        {CTRY_LITHUANIA, ETSI1_WORLD, "LT"},
        {CTRY_LUXEMBOURG, ETSI1_WORLD, "LU"},
        {CTRY_MACAU, FCC2_WORLD, "MO"},
        {CTRY_MACEDONIA, NULL1_WORLD, "MK"},
        {CTRY_MALAYSIA, APL8_WORLD, "MY"},
        {CTRY_MALTA, ETSI1_WORLD, "MT"},
        {CTRY_MEXICO, FCC1_FCCA, "MX"},
        {CTRY_MONACO, ETSI4_WORLD, "MC"},
        {CTRY_MOROCCO, NULL1_WORLD, "MA"},
        {CTRY_NEPAL, APL1_WORLD, "NP"},
        {CTRY_NETHERLANDS, ETSI1_WORLD, "NL"},
        {CTRY_NETHERLANDS_ANTILLES, ETSI1_WORLD, "AN"},
        {CTRY_NEW_ZEALAND, FCC2_ETSIC, "NZ"},
        {CTRY_NORWAY, ETSI1_WORLD, "NO"},
        {CTRY_OMAN, APL6_WORLD, "OM"},
        {CTRY_PAKISTAN, NULL1_WORLD, "PK"},
        {CTRY_PANAMA, FCC1_FCCA, "PA"},
       {CTRY_PAPUA_NEW_GUINEA, FCC1_WORLD, "PG"},
        {CTRY_PERU, APL1_WORLD, "PE"},
        {CTRY_PHILIPPINES, APL1_WORLD, "PH"},
        {CTRY_POLAND, ETSI1_WORLD, "PL"},
        {CTRY_PORTUGAL, ETSI1_WORLD, "PT"},
        {CTRY_PUERTO_RICO, FCC1_FCCA, "PR"},
        {CTRY_QATAR, NULL1_WORLD, "QA"},
        {CTRY_ROMANIA, NULL1_WORLD, "RO"},
        {CTRY_RUSSIA, NULL1_WORLD, "RU"},
        {CTRY_SAUDI_ARABIA, NULL1_WORLD, "SA"},
        {CTRY_SERBIA_MONTENEGRO, ETSI1_WORLD, "CS"},
        {CTRY_SINGAPORE, APL6_WORLD, "SG"},
        {CTRY_SLOVAKIA, ETSI1_WORLD, "SK"},
        {CTRY_SLOVENIA, ETSI1_WORLD, "SI"},
        {CTRY_SOUTH_AFRICA, FCC3_WORLD, "ZA"},
        {CTRY_SPAIN, ETSI1_WORLD, "ES"},
        {CTRY_SRI_LANKA, FCC3_WORLD, "LK"},
        {CTRY_SWEDEN, ETSI1_WORLD, "SE"},
        {CTRY_SWITZERLAND, ETSI1_WORLD, "CH"},
        {CTRY_SYRIA, NULL1_WORLD, "SY"},
        {CTRY_TAIWAN, APL3_FCCA, "TW"},
        {CTRY_THAILAND, FCC3_WORLD, "TH"},
        {CTRY_TRINIDAD_Y_TOBAGO, ETSI4_WORLD, "TT"},
        {CTRY_TUNISIA, ETSI3_WORLD, "TN"},
        {CTRY_TURKEY, ETSI3_WORLD, "TR"},
        {CTRY_UKRAINE, NULL1_WORLD, "UA"},
        {CTRY_UAE, NULL1_WORLD, "AE"},
        {CTRY_UNITED_KINGDOM, ETSI1_WORLD, "GB"},
        {CTRY_UNITED_STATES, FCC3_FCCA, "US"},
        /* This "PS" is for US public safety actually... to support this we
         * would need to assign new special alpha2 to CRDA db as with the world
         * regdomain and use another alpha2 */
        {CTRY_UNITED_STATES_FCC49, FCC4_FCCA, "PS"},
        {CTRY_URUGUAY, APL2_WORLD, "UY"},
        {CTRY_UZBEKISTAN, FCC3_FCCA, "UZ"},
        {CTRY_VENEZUELA, APL2_ETSIC, "VE"},
        {CTRY_VIET_NAM, NULL1_WORLD, "VN"},
        {CTRY_YEMEN, NULL1_WORLD, "YE"},
        {CTRY_ZIMBABWE, NULL1_WORLD, "ZW"},
};

/*
 * This is a set of common rules used by our world regulatory domains.
 * We have 12 world regulatory domains. To save space we consolidate
 * the regulatory domains in 5 structures by frequency and change
 * the flags on our reg_notifier() on a case by case basis.
 */

/* Only these channels all allow active scan on all world regulatory domains */
#define ATH9K_2GHZ_CH01_11		REG_RULE(2412-10, 2462+10, 40, 0, 20, 0)

/* We enable active scan on these a case by case basis by regulatory domain */
#define ATH9K_2GHZ_CH12_13		REG_RULE(2467-10, 2472+10, 40, 0, 20, NL80211_RRF_PASSIVE_SCAN)
#define ATH9K_2GHZ_CH14			REG_RULE(2484-10, 2484+10, 40, 0, 20, NL80211_RRF_PASSIVE_SCAN | NL80211_RRF_NO_OFDM)

/* We allow IBSS on these on a case by case basis by regulatory domain */
#define ATH9K_5GHZ_5150_5350	REG_RULE(5150-10, 5350+10, 40, 0, 30, NL80211_RRF_PASSIVE_SCAN | NL80211_RRF_NO_IBSS)
#define ATH9K_5GHZ_5470_5850	REG_RULE(5470-10, 5850+10, 40, 0, 30, NL80211_RRF_PASSIVE_SCAN | NL80211_RRF_NO_IBSS)
#define ATH9K_5GHZ_5725_5850	REG_RULE(5725-10, 5850+10, 40, 0, 30, NL80211_RRF_PASSIVE_SCAN | NL80211_RRF_NO_IBSS)

#define ATH9K_2GHZ_ALL			ATH9K_2GHZ_CH01_11, \
								ATH9K_2GHZ_CH12_13, \
								ATH9K_2GHZ_CH14

#define ATH9K_5GHZ_ALL			ATH9K_5GHZ_5150_5350, \
								ATH9K_5GHZ_5470_5850
							
/* This one skips what we call "mid band" */
#define ATH9K_5GHZ_NO_MIDBAND	ATH9K_5GHZ_5150_5350, \
								ATH9K_5GHZ_5725_5850

/* Can be used for:
 * 0x60, 0x61, 0x62 */
static const struct ieee80211_regdomain rtl_world_regdom_60_61_62 = {
	.n_reg_rules = 5,
	.alpha2 =  "99",
	.reg_rules = {
		ATH9K_2GHZ_ALL,
		ATH9K_5GHZ_ALL,
	}
};

/* Can be used by 0x63 and 0x65 */
static const struct ieee80211_regdomain rtl_world_regdom_63_65 = {
	.n_reg_rules = 4,
	.alpha2 =  "99",
	.reg_rules = {
		ATH9K_2GHZ_CH01_11,
		ATH9K_2GHZ_CH12_13,
		ATH9K_5GHZ_NO_MIDBAND,
	}
};

/* Can be used by 0x64 only */
static const struct ieee80211_regdomain rtl_world_regdom_64 = {
	.n_reg_rules = 3,
	.alpha2 =  "99",
	.reg_rules = {
		ATH9K_2GHZ_CH01_11,
		ATH9K_5GHZ_NO_MIDBAND,
	}
};

/* Can be used by 0x66 and 0x69 */
static const struct ieee80211_regdomain rtl_world_regdom_66_69 = {
	.n_reg_rules = 3,
	.alpha2 =  "99",
	.reg_rules = {
		ATH9K_2GHZ_CH01_11,
		ATH9K_5GHZ_ALL,
	}
};

/* Can be used by 0x67, 0x6A and 0x68 */
static const struct ieee80211_regdomain rtl_world_regdom_67_68_6A = {
	.n_reg_rules = 4,
	.alpha2 =  "99",
	.reg_rules = {
		ATH9K_2GHZ_CH01_11,
		ATH9K_2GHZ_CH12_13,
		ATH9K_5GHZ_ALL,
	}
};

#if 0
static inline bool is_wwr_sku(u16 regd)
{
	return ((regd & WORLD_SKU_MASK) == WORLD_SKU_PREFIX) ||
		(regd == WORLD);
}

static u16 rtl_regd_get_eepromRD(struct rtl_regulatory *reg)
{
	return reg->current_rd & ~WORLDWIDE_ROAMING_FLAG;
}

bool rtl_is_world_regd(struct rtl_regulatory *reg)
{
	return is_wwr_sku(rtl_regd_get_eepromRD(reg));
}
EXPORT_SYMBOL(rtl_is_world_regd);
#endif

static const struct ieee80211_regdomain *rtl_default_world_regdomain(void)
{
	/* this is the most restrictive */
	return &rtl_world_regdom_67_68_6A;
}

#if 0
static const struct
ieee80211_regdomain *rtl_world_regdomain(struct rtl_regulatory *reg)
{
	switch (reg->regpair->regDmnEnum) {
	case 0x60:
	case 0x61:
	case 0x62:
		return &rtl_world_regdom_60_61_62;
	case 0x63:
	case 0x65:
		return &rtl_world_regdom_63_65;
	case 0x64:
		return &rtl_world_regdom_64;
	case 0x66:
	case 0x69:
		return &rtl_world_regdom_66_69;
	case 0x67:
	case 0x68:
	case 0x6A:
		return &rtl_world_regdom_67_68_6A;
	default:
		WARN_ON(1);
		return rtl_default_world_regdomain();
	}
}
#endif

/* Frequency is one where radar detection is required */
static bool rtl_is_radar_freq(u16 center_freq)
{
	return (center_freq >= 5260 && center_freq <= 5700);
}

/*
 * N.B: These exception rules do not apply radar freqs.
 *
 * - We enable adhoc (or beaconing) if allowed by 11d
 * - We enable active scan if the channel is allowed by 11d
 * - If no country IE has been processed and we determine we have
 *   received a beacon on a channel we can enable active scan and
 *   adhoc (or beaconing).
 */
static void
rtl_reg_apply_beaconing_flags(struct wiphy *wiphy,
			      enum nl80211_reg_initiator initiator)
{
	enum ieee80211_band band;
	struct ieee80211_supported_band *sband;
	const struct ieee80211_reg_rule *reg_rule;
	struct ieee80211_channel *ch;
	unsigned int i;
	u32 bandwidth = 0;
	int r;

	for (band = 0; band < IEEE80211_NUM_BANDS; band++) {

		if (!wiphy->bands[band])
			continue;

		sband = wiphy->bands[band];

		for (i = 0; i < sband->n_channels; i++) {

			ch = &sband->channels[i];

			if (rtl_is_radar_freq(ch->center_freq) ||
			    (ch->flags & IEEE80211_CHAN_RADAR))
				continue;

			if (initiator == NL80211_REGDOM_SET_BY_COUNTRY_IE) {
				r = freq_reg_info(wiphy,
						  ch->center_freq,
						  bandwidth,
						  &reg_rule);
				if (r)
					continue;
				/*
				 * If 11d had a rule for this channel ensure
				 * we enable adhoc/beaconing if it allows us to
				 * use it. Note that we would have disabled it
				 * by applying our static world regdomain by
				 * default during init, prior to calling our
				 * regulatory_hint().
				 */
				if (!(reg_rule->flags &
				    NL80211_RRF_NO_IBSS))
					ch->flags &=
					  ~IEEE80211_CHAN_NO_IBSS;
				if (!(reg_rule->flags &
				    NL80211_RRF_PASSIVE_SCAN))
					ch->flags &=
					  ~IEEE80211_CHAN_PASSIVE_SCAN;
			} else {
				if (ch->beacon_found)
					ch->flags &= ~(IEEE80211_CHAN_NO_IBSS |
					  IEEE80211_CHAN_PASSIVE_SCAN);
			}
		}
	}

}

/* Allows active scan scan on Ch 12 and 13 */
static void
rtl_reg_apply_active_scan_flags(struct wiphy *wiphy,
				enum nl80211_reg_initiator initiator)
{
	struct ieee80211_supported_band *sband;
	struct ieee80211_channel *ch;
	const struct ieee80211_reg_rule *reg_rule;
	u32 bandwidth = 0;
	int r;

	sband = wiphy->bands[IEEE80211_BAND_2GHZ];

	/*
	 * If no country IE has been received always enable active scan
	 * on these channels. This is only done for specific regulatory SKUs
	 */
	if (initiator != NL80211_REGDOM_SET_BY_COUNTRY_IE) {
		ch = &sband->channels[11]; /* CH 12 */
		if (ch->flags & IEEE80211_CHAN_PASSIVE_SCAN)
			ch->flags &= ~IEEE80211_CHAN_PASSIVE_SCAN;
		ch = &sband->channels[12]; /* CH 13 */
		if (ch->flags & IEEE80211_CHAN_PASSIVE_SCAN)
			ch->flags &= ~IEEE80211_CHAN_PASSIVE_SCAN;
		return;
	}

	/*
	 * If a country IE has been recieved check its rule for this
	 * channel first before enabling active scan. The passive scan
	 * would have been enforced by the initial processing of our
	 * custom regulatory domain.
	 */

	ch = &sband->channels[11]; /* CH 12 */
	r = freq_reg_info(wiphy, ch->center_freq, bandwidth, &reg_rule);
	if (!r) {
		if (!(reg_rule->flags & NL80211_RRF_PASSIVE_SCAN))
			if (ch->flags & IEEE80211_CHAN_PASSIVE_SCAN)
				ch->flags &= ~IEEE80211_CHAN_PASSIVE_SCAN;
	}

	ch = &sband->channels[12]; /* CH 13 */
	r = freq_reg_info(wiphy, ch->center_freq, bandwidth, &reg_rule);
	if (!r) {
		if (!(reg_rule->flags & NL80211_RRF_PASSIVE_SCAN))
			if (ch->flags & IEEE80211_CHAN_PASSIVE_SCAN)
				ch->flags &= ~IEEE80211_CHAN_PASSIVE_SCAN;
	}
}

/* Always apply Radar/DFS rules on freq range 5260 MHz - 5700 MHz */
static void rtl_reg_apply_radar_flags(struct wiphy *wiphy)
{
	struct ieee80211_supported_band *sband;
	struct ieee80211_channel *ch;
	unsigned int i;

	if (!wiphy->bands[IEEE80211_BAND_5GHZ])
		return;

	sband = wiphy->bands[IEEE80211_BAND_5GHZ];

	for (i = 0; i < sband->n_channels; i++) {
		ch = &sband->channels[i];
		if (!rtl_is_radar_freq(ch->center_freq))
			continue;
		/* We always enable radar detection/DFS on this
		 * frequency range. Additionally we also apply on
		 * this frequency range:
		 * - If STA mode does not yet have DFS supports disable
		 *   active scanning
		 * - If adhoc mode does not support DFS yet then
		 *   disable adhoc in the frequency.
		 * - If AP mode does not yet support radar detection/DFS
		 *   do not allow AP mode
		 */
		if (!(ch->flags & IEEE80211_CHAN_DISABLED))
			ch->flags |= IEEE80211_CHAN_RADAR |
				     IEEE80211_CHAN_NO_IBSS |
				     IEEE80211_CHAN_PASSIVE_SCAN;
	}
}

static void rtl_reg_apply_world_flags(struct wiphy *wiphy,
				      enum nl80211_reg_initiator initiator,
				      struct rtl_regulatory *reg)
{
#if 0
	switch (reg->regpair->regDmnEnum) {
	case 0x60:
	case 0x63:
	case 0x66:
	case 0x67:
		rtl_reg_apply_beaconing_flags(wiphy, initiator);
		break;
	case 0x68:
		rtl_reg_apply_beaconing_flags(wiphy, initiator);
		rtl_reg_apply_active_scan_flags(wiphy, initiator);
		break;
	}
#else
	rtl_reg_apply_beaconing_flags(wiphy, initiator);
#endif
	return;
}

int rtl_reg_notifier_apply(struct wiphy *wiphy,
			   struct regulatory_request *request,
			   struct rtl_regulatory *reg)
{
	/* We always apply this */
	rtl_reg_apply_radar_flags(wiphy);

	switch (request->initiator) {
	case NL80211_REGDOM_SET_BY_DRIVER:
	case NL80211_REGDOM_SET_BY_CORE:
	case NL80211_REGDOM_SET_BY_USER:
		rtl_reg_apply_world_flags(wiphy, request->initiator, reg);
		break;
	case NL80211_REGDOM_SET_BY_COUNTRY_IE:
		rtl_reg_apply_world_flags(wiphy, request->initiator, reg);
		break;
	}

	rtl_dump_channel_map(wiphy);
	return 0;
}

#if 0
static bool rtl_regd_is_eeprom_valid(struct rtl_regulatory *reg)
{
	 u16 rd = rtl_regd_get_eepromRD(reg);
	int i;

	if (rd & COUNTRY_ERD_FLAG) {
		/* EEPROM value is a country code */
		u16 cc = rd & ~COUNTRY_ERD_FLAG;
		printk(KERN_DEBUG
		       "rtl: EEPROM indicates we should expect "
			"a country code\n");
		for (i = 0; i < ARRAY_SIZE(allCountries); i++)
			if (allCountries[i].countryCode == cc)
				return true;
	} else {
		/* EEPROM value is a regpair value */
		if (rd != CTRY_DEFAULT)
			printk(KERN_DEBUG "rtl: EEPROM indicates we "
			       "should expect a direct regpair map\n");
		for (i = 0; i < ARRAY_SIZE(regDomainPairs); i++)
			if (regDomainPairs[i].regDmnEnum == rd)
				return true;
	}
	printk(KERN_DEBUG
		 "rtl: invalid regulatory domain/country code 0x%x\n", rd);
	return false;
}

/* EEPROM country code to regpair mapping */
static struct country_code_to_enum_rd*
rtl_regd_find_country(u16 countryCode)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(allCountries); i++) {
		if (allCountries[i].countryCode == countryCode)
			return &allCountries[i];
	}
	return NULL;
}

/* EEPROM rd code to regpair mapping */
static struct country_code_to_enum_rd*
rtl_regd_find_country_by_rd(int regdmn)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(allCountries); i++) {
		if (allCountries[i].regDmnEnum == regdmn)
			return &allCountries[i];
	}
	return NULL;
}

/* Returns the map of the EEPROM set RD to a country code */
static u16 rtl_regd_get_default_country(u16 rd)
{
	if (rd & COUNTRY_ERD_FLAG) {
		struct country_code_to_enum_rd *country = NULL;
		u16 cc = rd & ~COUNTRY_ERD_FLAG;

		country = rtl_regd_find_country(cc);
		if (country != NULL)
			return cc;
	}

	return CTRY_DEFAULT;
}
#endif

static struct reg_dmn_pair_mapping*
rtl_get_regpair(int regdmn)
{
	int i;

	if (regdmn == NO_ENUMRD)
		return NULL;
	for (i = 0; i < ARRAY_SIZE(regDomainPairs); i++) {
		if (regDomainPairs[i].regDmnEnum == regdmn)
			return &regDomainPairs[i];
	}
	return NULL;
}


static int
rtl_regd_init_wiphy(struct rtl_regulatory *reg,
		    struct wiphy *wiphy,
		    int (*reg_notifier)(struct wiphy *wiphy,
					struct regulatory_request *request))
{
	const struct ieee80211_regdomain *regd;

	wiphy->reg_notifier = reg_notifier;
	wiphy->custom_regulatory = true;
	wiphy->strict_regulatory = false;
	wiphy->disable_beacon_hints = false;

#if 0
	if (rtl_is_world_regd(reg)) {
		/*
		 * Anything applied here (prior to wiphy registration) gets
		 * saved on the wiphy orig_* parameters
		 */
		regd = rtl_world_regdomain(reg);
		wiphy->custom_regulatory = true;
		wiphy->strict_regulatory = false;
	} else {
		/*
		 * This gets applied in the case of the absense of CRDA,
		 * it's our own custom world regulatory domain, similar to
		 * cfg80211's but we enable passive scanning.
		 */
		regd = rtl_default_world_regdomain();
	}
#else
	regd = rtl_default_world_regdomain();

#endif
	wiphy_apply_custom_regulatory(wiphy, regd);
	rtl_reg_apply_radar_flags(wiphy);
	rtl_reg_apply_world_flags(wiphy, NL80211_REGDOM_SET_BY_DRIVER, reg);
	return 0;
}

#if 0
/*
 * Some users have reported their EEPROM programmed with
 * 0x8000 set, this is not a supported regulatory domain
 * but since we have more than one user with it we need
 * a solution for them. We default to 0x64, which is the
 * default Atheros world regulatory domain.
 */
static void rtl_regd_sanitize(struct rtl_regulatory *reg)
{
	if (reg->current_rd != COUNTRY_ERD_FLAG)
		return;
	printk(KERN_DEBUG "rtl: EEPROM regdomain sanitized\n");
	reg->current_rd = 0x64;
}
#endif

void rtl_dump_channel_map(struct wiphy *wiphy)
{
	enum ieee80211_band band;
	struct ieee80211_supported_band *sband;
	struct ieee80211_channel *ch;
	unsigned int i;

	for (band = 0; band < IEEE80211_NUM_BANDS; band++) {

		if (!wiphy->bands[band])
			continue;

		sband = wiphy->bands[band];

		for (i = 0; i < sband->n_channels; i++) {
			ch = &sband->channels[i];
			printk("###>%s(), chan:%d, NO_IBSS:%d," 
					" PASSIVE_SCAN:%d, RADAR:%d, DISABLED:%d\n", __func__,i+1,
					ch->flags&IEEE80211_CHAN_NO_IBSS, 
					ch->flags&IEEE80211_CHAN_PASSIVE_SCAN,
					ch->flags&IEEE80211_CHAN_RADAR,
					ch->flags&IEEE80211_CHAN_DISABLED
			      );
		}

	}
}
int
rtl_regd_init(struct net_device *dev,
	      int (*reg_notifier)(struct wiphy *wiphy,
				  struct regulatory_request *request))
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);	
	struct rtl_regulatory *reg = &priv->rtllib->regulatory;
	struct wiphy *wiphy = priv->rtllib->wdev.wiphy;
	struct country_code_to_enum_rd *country = NULL;
	u16 regdmn;
	printk("####################>%s()\n", __func__);
	if (wiphy == NULL || reg == NULL){
		printk(">>>>>>>>>>>>>>..>%s()\n", __func__);
		return -EINVAL;
	}

#if 0
	rtl_regd_sanitize(reg);

	printk(KERN_DEBUG "rtl: EEPROM regdomain: 0x%0x\n", reg->current_rd);

	if (!rtl_regd_is_eeprom_valid(reg)) {
		printk(KERN_ERR "rtl: Invalid EEPROM contents\n");
		return -EINVAL;
	}

	regdmn = rtl_regd_get_eepromRD(reg);
	reg->country_code = rtl_regd_get_default_country(regdmn);
#else
	regdmn = CTRY_DEFAULT;
	reg->country_code = CTRY_DEFAULT;
#endif

	if (reg->country_code == CTRY_DEFAULT && regdmn == CTRY_DEFAULT) {
		printk(KERN_DEBUG "rtl: EEPROM indicates default "
		       "country code should be used\n");
		reg->country_code = CTRY_UNITED_STATES;
	}

	if (reg->country_code == CTRY_DEFAULT) {
		country = NULL;
	} else {
#if 0
		printk(KERN_DEBUG "rtl: doing EEPROM country->regdmn "
		       "map search\n");
		country = rtl_regd_find_country(reg->country_code);
		if (country == NULL) {
			printk(KERN_DEBUG
				"rtl: no valid country maps found for "
				"country code: 0x%0x\n",
				reg->country_code);
			return -EINVAL;
		} else {
			regdmn = country->regDmnEnum;
			printk(KERN_DEBUG "rtl: country maps to "
			       "regdmn code: 0x%0x\n",
			       regdmn);
		}
#endif
	}

#if 0
	reg->regpair = rtl_get_regpair(regdmn);

	if (!reg->regpair) {
		printk(KERN_DEBUG "rtl: "
			"No regulatory domain pair found, cannot continue\n");
		return -EINVAL;
	}

	if (!country)
		country = rtl_regd_find_country_by_rd(regdmn);
#endif

	if (country) {
		reg->alpha2[0] = country->isoName[0];
		reg->alpha2[1] = country->isoName[1];
	} else {
		reg->alpha2[0] = '0';
		reg->alpha2[1] = '0';
	}

	printk(KERN_DEBUG "rtl: Country alpha2 being used: %c%c\n",
		reg->alpha2[0], reg->alpha2[1]);


	rtl_regd_init_wiphy(reg, wiphy, reg_notifier);
	rtl_dump_channel_map(wiphy);
	return 0;
}

#if 0
u32 rtl_regd_get_band_ctl(struct rtl_regulatory *reg,
			  enum ieee80211_band band)
{
	if (!reg->regpair ||
	    (reg->country_code == CTRY_DEFAULT &&
	     is_wwr_sku(rtl_regd_get_eepromRD(reg)))) {
		return SD_NO_CTL;
	}

	switch (band) {
	case IEEE80211_BAND_2GHZ:
		return reg->regpair->reg_2ghz_ctl;
	case IEEE80211_BAND_5GHZ:
		return reg->regpair->reg_5ghz_ctl;
	default:
		return NO_CTL;
	}
}
EXPORT_SYMBOL(rtl_regd_get_band_ctl);
#endif

int rtl_reg_notifier(struct wiphy *wiphy,
			      struct regulatory_request *request)
{
	struct net_device *dev = wiphy_to_net_device(wiphy);
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);	
	struct rtl_regulatory *reg = &priv->rtllib->regulatory;
	printk("################>%s\n", __func__);

	return rtl_reg_notifier_apply(wiphy, request, reg);
}

struct net_device *wiphy_to_net_device(struct wiphy *wiphy)
{
	struct rtllib_device *rtllib;

	rtllib = wiphy_priv(wiphy);
	return rtllib->dev;
}

static const struct ieee80211_rate rtl819x_rates[] = {
	{ .bitrate = 10, .hw_value = 0, },
	{ .bitrate = 20, .hw_value = 1, },
	{ .bitrate = 55, .hw_value = 2, },
	{ .bitrate = 110, .hw_value = 3, },
	{ .bitrate = 60, .hw_value = 4, },
	{ .bitrate = 90, .hw_value = 5, },
	{ .bitrate = 120, .hw_value = 6, },
	{ .bitrate = 180, .hw_value = 7, },
	{ .bitrate = 240, .hw_value = 8, },
	{ .bitrate = 360, .hw_value = 9, },
	{ .bitrate = 480, .hw_value = 10, },
	{ .bitrate = 540, .hw_value = 11, },
};

#define CHAN2G(_freq, _idx)  { \
	        .band = IEEE80211_BAND_2GHZ, \
	        .center_freq = (_freq), \
	        .hw_value = (_idx), \
	        .max_power = 20, \
}

static struct ieee80211_channel rtl819x_2ghz_chantable[] = {
	CHAN2G(2412, 1), /* Channel 1 */
	CHAN2G(2417, 2), /* Channel 2 */
	CHAN2G(2422, 3), /* Channel 3 */
	CHAN2G(2427, 4), /* Channel 4 */
	CHAN2G(2432, 5), /* Channel 5 */
	CHAN2G(2437, 6), /* Channel 6 */
	CHAN2G(2442, 7), /* Channel 7 */
	CHAN2G(2447, 8), /* Channel 8 */
	CHAN2G(2452, 9), /* Channel 9 */
	CHAN2G(2457, 10), /* Channel 10 */
	CHAN2G(2462, 11), /* Channel 11 */
	CHAN2G(2467, 12), /* Channel 12 */
	CHAN2G(2472, 13), /* Channel 13 */
	CHAN2G(2484, 14), /* Channel 14 */
};

int rtllib_set_geo(struct r8192_priv *priv)
{	
	priv->bands[IEEE80211_BAND_2GHZ].band = IEEE80211_BAND_2GHZ;
	priv->bands[IEEE80211_BAND_2GHZ].channels = rtl819x_2ghz_chantable;
	priv->bands[IEEE80211_BAND_2GHZ].n_channels = ARRAY_SIZE(rtl819x_2ghz_chantable);
	
	memcpy(&priv->rates[IEEE80211_BAND_2GHZ], rtl819x_rates, sizeof(rtl819x_rates));

	priv->bands[IEEE80211_BAND_2GHZ].n_bitrates = ARRAY_SIZE(rtl819x_rates);
	priv->bands[IEEE80211_BAND_2GHZ].bitrates = priv->rates[IEEE80211_BAND_2GHZ];

	return 0;
}

bool rtl8192_register_wiphy_dev(struct net_device *dev)
{	
	struct r8192_priv *priv = rtllib_priv(dev);
	struct wireless_dev *wdev = &priv->rtllib->wdev;
	struct rtl_regulatory *reg;

	memcpy(wdev->wiphy->perm_addr, dev->dev_addr, ETH_ALEN);
	wdev->wiphy->bands[IEEE80211_BAND_2GHZ] = &(priv->bands[IEEE80211_BAND_2GHZ]);
	set_wiphy_dev(wdev->wiphy, &priv->pdev->dev);

	/* With that information in place, we can now register the wiphy... */
	if (wiphy_register(wdev->wiphy)) {
		return false;
	}

	if(rtl_regd_init(dev, rtl_reg_notifier))
		return false;

	reg = &priv->rtllib->regulatory;
	if(reg != NULL){
		if(regulatory_hint(wdev->wiphy, reg->alpha2)){
			printk("########>%s() regulatory_hint fail\n", __func__);
			;
		}else{
			printk("########>#%s() regulatory_hint success\n", __func__);
		}
	}else{
		printk("#########%s() regulator null\n", __func__);
	}
		
	return true;
}

#endif
