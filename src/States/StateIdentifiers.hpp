#ifndef FREESHOP_STATEIDENTIFIERS_HPP
#define FREESHOP_STATEIDENTIFIERS_HPP

namespace FreeShop {
	namespace States {
		enum ID {
			None,
			Title,
			Loading,
			Sync,
			Browse,
			Sleep,
			QrScanner,
			Dialog,
			News,
		};
	}
}

#endif // FREESHOP_STATEIDENTIFIERS_HPP
