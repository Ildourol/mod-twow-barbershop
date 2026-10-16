-- ============================================================================
--  Barbershop Chair Script Binding (tw_world)
-- ============================================================================

-- Assign the go_barber_chair script to the standard Turtle WoW Barbershop Chair
UPDATE `gameobject_template`
SET `script_name` = 'go_barber_chair'
WHERE `entry` = 105180;
