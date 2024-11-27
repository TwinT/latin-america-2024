import m5
from m5.objects.HelloSimObject import HelloSimObject
from m5.objects.Root import Root

root = Root(full_system=False)
root.hello = HelloSimObject(num_hellos=5)

m5.instantiate()
exit_event = m5.simulate()

print(f"Exited simulation because: {exit_event.getCause()}.")
