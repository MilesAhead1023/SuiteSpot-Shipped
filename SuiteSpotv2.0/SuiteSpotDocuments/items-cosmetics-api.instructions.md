# ItemsWrapper & LoadoutWrapper Complete API Reference

**AI Agent Reference Guide - For Copilot CLI Auto-Loading**

ItemsWrapper and LoadoutWrapper manage player inventory, cosmetics, and loadouts in Rocket League. ItemsWrapper provides access to all products and databases, while LoadoutWrapper represents a single configuration of items.

## ItemsWrapper Overview

ItemsWrapper gives you access to:
- All products in inventory
- Product slots and loadouts
- Cosmetic databases (paint, certified, special edition)
- Owned/unlocked products
- Trade system
- Current loadout access

## ItemsWrapper Constructor/Initialization

Get via GameWrapper:

```cpp
auto gameWrapper = GetGameWrapper();
auto itemsWrapper = gameWrapper->GetItemsWrapper();
if (!itemsWrapper) return;

// Access all products, loadouts, etc.
```

## ItemsWrapper Product Access

### Get Products
```cpp
// Get all products in system
ArrayWrapper<ProductWrapper> allProducts = itemsWrapper->GetAllProducts();
for (int i = 0; i < allProducts.Count(); i++) {
    auto product = allProducts.Get(i);
    if (!product) continue;
    // Process product
}

// Get product by ID
ProductWrapper product = itemsWrapper->GetProduct(12345);
if (product) {
    // Use product
}

// Get cached unlocked products
ArrayWrapper<ProductWrapper> unlockedProducts = itemsWrapper->GetCachedUnlockedProducts();

// Get owned products online
ArrayWrapper<OnlineProductWrapper> ownedProducts = itemsWrapper->GetOwnedProducts();
```

### Product Slots
```cpp
// Get all product slots (inventory categories)
ArrayWrapper<ProductSlotWrapper> allSlots = itemsWrapper->GetAllProductSlots();
for (int i = 0; i < allSlots.Count(); i++) {
    auto slot = allSlots.Get(i);
    if (!slot) continue;
    // Process slot
}
```

## ItemsWrapper Databases

### Access Cosmetic Databases
```cpp
// Get certified stat database
CertifiedStatDatabaseWrapper certDB = itemsWrapper->GetCertifiedStatDB();

// Get paint database
PaintDatabaseWrapper paintDB = itemsWrapper->GetPaintDB();

// Get special edition database
SpecialEditionDatabaseWrapper seDB = itemsWrapper->GetSpecialEditionDB();

// Get esports team database
DataAssetDatabase_ESportsTeamWrapper esportsDB = itemsWrapper->GetEsportTeamDB();
```

### Graphics Products
```cpp
// Get GFX products wrapper
GfxProductsWrapper gfxProducts = itemsWrapper->GetGfxProductsWrapper();
```

## ItemsWrapper Trade System

```cpp
// Get trade wrapper
TradeWrapper trades = itemsWrapper->GetTradeWrapper();

// Get product trade-in wrapper
ProductTradeInWrapper tradeIn = itemsWrapper->GetProductTradeInWrapper();
```

## ItemsWrapper Loadout Access

### Current Loadout
```cpp
// Get current loadout name
UnrealStringWrapper loadoutName = itemsWrapper->GetCurrentLoadoutName();
std::string name = loadoutName.ToString();

// Get current loadout for team (0 = blue, 1 = orange)
LoadoutWrapper teamLoadout = itemsWrapper->GetCurrentLoadout(0);  // Blue team
if (teamLoadout) {
    // Access loadout items
}

// Get current orange team loadout
LoadoutWrapper orangeLoadout = itemsWrapper->GetCurrentLoadout(1);
```

---

## LoadoutWrapper Overview

LoadoutWrapper represents a single car loadout (configuration of cosmetics). Contains what car body, wheels, decal, paint, etc. are selected.

## LoadoutWrapper Constructor/Initialization

Get via ItemsWrapper or PriWrapper:

```cpp
auto gameWrapper = GetGameWrapper();
auto itemsWrapper = gameWrapper->GetItemsWrapper();
if (!itemsWrapper) return;

// Get loadout for team
auto loadout = itemsWrapper->GetCurrentLoadout(0);  // Team 0
if (!loadout) return;

// Or via PriWrapper
auto player = ...;  // Get PriWrapper
auto playerLoadout = player->GetLoadout();
```

## LoadoutWrapper Item Access

### Get Loadout Items
```cpp
// Get all items in loadout (as product IDs)
ArrayWrapper<int> loadoutItems = loadout->GetLoadout();
for (int i = 0; i < loadoutItems.Count(); i++) {
    int productID = loadoutItems.Get(i);
    // Product ID refers to items in ItemsWrapper
}

// Get online loadout (deprecated - missing data)
// Don't use: GetOnlineLoadout()

// Get online loadout V2 (recommended)
std::vector<ProductInstanceID> onlineLoadoutV2 = loadout->GetOnlineLoadoutV2();
for (const auto& productID : onlineLoadoutV2) {
    // Access product instance
}
```

## LoadoutWrapper Paint/Finish Access

### Colors & Paint
```cpp
// Get primary paint color ID (0-255)
unsigned char primaryPaint = loadout->GetPrimaryPaintColorId();

// Get accent paint color ID (0-255)
unsigned char accentPaint = loadout->GetAccentPaintColorId();

// Get primary finish ID (paint texture/type)
int primaryFinish = loadout->GetPrimaryFinishId();

// Get accent finish ID
int accentFinish = loadout->GetAccentFinishId();
```

## Common Patterns

### Get Current Car Loadout for Local Player
```cpp
gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto itemsWrapper = gw->GetItemsWrapper();
    if (!itemsWrapper) return;
    
    // Get local player's current team (blue=0, orange=1)
    auto car = gw->GetLocalCar();
    if (!car) return;
    int team = car->GetTeamNum();
    
    // Get that team's loadout
    auto loadout = itemsWrapper->GetCurrentLoadout(team);
    if (!loadout) return;
    
    // Get primary paint
    unsigned char paintID = loadout->GetPrimaryPaintColorId();
    int finishID = loadout->GetPrimaryFinishId();
    
    std::cout << "Current loadout paint: " << (int)paintID 
              << ", finish: " << finishID << "\n";
}, 1.0f);
```

### List All Products
```cpp
gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto itemsWrapper = gw->GetItemsWrapper();
    if (!itemsWrapper) return;
    
    auto allProducts = itemsWrapper->GetAllProducts();
    std::cout << "Total products: " << allProducts.Count() << "\n";
    
    // Show first 10 products
    for (int i = 0; i < std::min(10, allProducts.Count()); i++) {
        auto product = allProducts.Get(i);
        if (product) {
            // Access product data
        }
    }
}, 5.0f);
```

### Get Both Team Loadouts
```cpp
gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto itemsWrapper = gw->GetItemsWrapper();
    if (!itemsWrapper) return;
    
    auto blueLoadout = itemsWrapper->GetCurrentLoadout(0);
    auto orangeLoadout = itemsWrapper->GetCurrentLoadout(1);
    
    if (blueLoadout && orangeLoadout) {
        auto bluePaint = blueLoadout->GetPrimaryPaintColorId();
        auto orangePaint = orangeLoadout->GetPrimaryPaintColorId();
        
        std::cout << "Blue paint: " << (int)bluePaint 
                  << ", Orange paint: " << (int)orangePaint << "\n";
    }
}, 5.0f);
```

### Check Owned Products
```cpp
gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto itemsWrapper = gw->GetItemsWrapper();
    if (!itemsWrapper) return;
    
    auto owned = itemsWrapper->GetOwnedProducts();
    std::cout << "Owned products: " << owned.Count() << "\n";
    
    // Can iterate through owned products
    for (int i = 0; i < owned.Count() && i < 5; i++) {
        auto product = owned.Get(i);
        // Process product
    }
}, 10.0f);
```

## ProductInstanceID Handling

```cpp
// Check if ID is online ID
bool isOnlineID = ItemsWrapper::IsOnlineID(productID);

// Get product ID from instance ID
int productID = ItemsWrapper::GetProductIdFromInstanceId(productID);
```

## Thread Safety Notes

- **Safe to call from**: Game thread (inside hooks, SetTimeout callbacks)
- **Not safe to call from**: Other threads without synchronization
- **Wrapper lifetime**: Valid for duration of match; invalid after disconnect
- **Always null-check**: `if (!itemsWrapper) return;` before using

```cpp
// SAFE
gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto itemsWrapper = gw->GetItemsWrapper();
    if (!itemsWrapper) return;  // NULL CHECK
    
    auto products = itemsWrapper->GetAllProducts();
    int count = products.Count();
}, 1.0f);

// UNSAFE - Storing wrapper
auto cachedItems = itemsWrapper;
// ... later ...
auto products = cachedItems->GetAllProducts();  // Might be invalid!
```

## Performance Considerations

- **GetAllProducts()** returns all products; can be large
- **GetCurrentLoadout()** is fast; safe to call every frame
- **Iterating products** is efficient if you cache the array
- **Paint/finish lookups** are O(1); no performance concern

```cpp
// EFFICIENT
auto itemsWrapper = gw->GetItemsWrapper();
auto allProducts = itemsWrapper->GetAllProducts();  // Cache once
for (int i = 0; i < allProducts.Count(); i++) {
    auto product = allProducts.Get(i);
    // Use product
}

// INEFFICIENT
for (int i = 0; i < gw->GetItemsWrapper()->GetAllProducts().Count(); i++) {
    // GetAllProducts() called each iteration
}
```

## SuiteSpot-Specific Usage

While SuiteSpot doesn't access cosmetics, you could extend it:

```cpp
// Example: Verify loadouts are loaded
gameWrapper->HookEventPost(
    "Function TAGame.GameEvent_Soccar_TA.EventBegin",
    [this](void* params) {
        auto itemsWrapper = GetGameWrapper()->GetItemsWrapper();
        if (itemsWrapper) {
            auto loadout = itemsWrapper->GetCurrentLoadout(0);
            if (loadout) {
                // Loadouts are ready
            }
        }
    }
);
```

## Important Notes for AI Agents

1. **Null-check wrappers** - Items/loadouts might not be ready immediately
2. **ProductInstanceID vs ProductID** - Different identifier types; don't mix
3. **Team index is 0 or 1** - Blue=0, Orange=1 for loadouts
4. **Paint ID is 0-255** - Limited range; maps to paint database
5. **Finish ID varies** - Different paint types have different finish ranges
6. **Databases are queryable** - Use Paint/Certified/SpecialEdition databases
7. **GetOnlineLoadout() is deprecated** - Use GetOnlineLoadoutV2() instead
8. **Loadouts are team-specific** - Each team has separate loadout selections
9. **Product count can be large** - Cache if iterating frequently
10. **Trade system is accessible** - Can query trades via TradeWrapper

---
**Last Updated:** 2025-12-14
**For:** AI Coding Agents
**Format:** Strictly `.instructions.md` for auto-loading
**SDK Source:** BakkesMod SDK include/bakkesmod/wrappers/items/ (ItemsWrapper.h, LoadoutWrapper.h)
