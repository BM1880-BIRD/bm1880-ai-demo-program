#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/platform_device.h>


#define BM1880_GPIO(x) (x<32)?(480+x):((x<64)?(448+x-32):((x<=67)?(444+x-64):(-1)))

#define BM_GPIO_KEY_CLK		BM1880_GPIO(57)
#define BM_GPIO_KEY_DATA	BM1880_GPIO(63)
#define BM_GPIO_KEY_KB1		BM1880_GPIO(64)
#define BM_GPIO_KEY_KB2		BM1880_GPIO(65)

#define KB_BUTTON_DOWN (0)
#define KB_BUTTON_UP (1)
#define KB_BUTTON_NONE (0xFFFFFFFF)



typedef struct
{
	struct input_dev *input_keypd_dev ;
	struct delayed_work work;
	spinlock_t lock;
	int Kb1;
	int	Kb2;
	int irq;
	
}keypad_74x164;


void bm1880_74x164_shift1bit(int Value)
{	
	gpio_direction_output(BM_GPIO_KEY_CLK, 0);	
	gpio_direction_output(BM_GPIO_KEY_DATA, Value);	
	gpio_direction_output(BM_GPIO_KEY_CLK, 1);
}


void bm1880_keypad_74x164_out_value(unsigned char Value)
{
	unsigned char u1Val = 0x80;
	int i;

	for(i=0; i<8; i++) {
		//printk("u1Val = 0x%x .\n", u1Val);
		bm1880_74x164_shift1bit(Value & u1Val);
		u1Val = u1Val >> 1;
	}
}

int keypad_74x164_getkey(int Kb1, int Kb2)
{
	unsigned char u1Val;
	int Kb1Value, Kb2Value;
	int key_code = KEY_RESERVED;
	int i;
	
	if(Kb1 == KB_BUTTON_DOWN) {
		u1Val = 0x80;
		for(i=0; i<=8; i++) {
			//printk("KB1 xxx = 0x%X .\n",~u1Val);
			bm1880_keypad_74x164_out_value(~u1Val);
			Kb1Value = gpio_get_value(BM_GPIO_KEY_KB1);
			if(Kb1Value == 0) {
				break;
			}
			u1Val = u1Val >> 1;
		}
		switch(u1Val) {
			case 0x01:
				key_code = KEY_0;
				break;
			case 0x02:
				key_code = KEY_1;
				break;
			case 0x04:
				key_code = KEY_2;
				break;
			case 0x08:
				key_code = KEY_3;
				break;
			case 0x10:
				key_code = KEY_4;
				break;
			case 0x20:
				key_code = KEY_5;
				break;
			case 0x40:
				key_code = KEY_6;
				break;
			case 0x80:
				key_code = KEY_7;
				break;
			default:
				key_code = KEY_RESERVED;
				break;
		}
		bm1880_keypad_74x164_out_value(0x00);
	}

	if(Kb2 == 0) {
		u1Val = 0x80;
		for(i=0; i<=8; i++) {
			//printk("KB2 xxx = 0x%X .\n",~u1Val);
			bm1880_keypad_74x164_out_value(~u1Val);
			Kb2Value = gpio_get_value(BM_GPIO_KEY_KB2);
			if(Kb2Value == 0) {
				break;
			}
			u1Val = u1Val >> 1;
		}
		switch(u1Val) {
			case 0x01:
				key_code = KEY_8;
				break;
			case 0x02:
				key_code = KEY_9;
				break;
			case 0x04:
				key_code = KEY_A;
				break;
			case 0x08:
				key_code = KEY_B;
				break;
			case 0x10:
				key_code = KEY_C;
				break;
			case 0x20:
				key_code = KEY_D;
				break;
			case 0x40:
				key_code = KEY_E;
				break;
			case 0x80:
				key_code = KEY_F;
				break;
			default:
				key_code = KEY_RESERVED;
				break;
		}
		bm1880_keypad_74x164_out_value(0x00);
	}
	return key_code;
}




irqreturn_t keypad_74x164_irq_service(int irq, void *para)
{
	unsigned long irq_flags = 0;
	keypad_74x164 * keypad_data = (keypad_74x164 *)para;

	spin_lock_irqsave(&(keypad_data->lock), irq_flags);
	//local_irq_save(irq_flags);

	if (irq == gpio_to_irq(BM_GPIO_KEY_KB1)) {
		keypad_data->Kb1 = gpio_get_value(BM_GPIO_KEY_KB1);
		keypad_data->Kb2 = KB_BUTTON_NONE;

	} else if(irq == gpio_to_irq(BM_GPIO_KEY_KB2)) {
		keypad_data->Kb1 = KB_BUTTON_NONE;
		keypad_data->Kb2 = gpio_get_value(BM_GPIO_KEY_KB2);
	}
	
	keypad_data->irq = irq;

	//printk("%s: %d , irq = %d , [%d][%d]\n", __FUNCTION__, __LINE__, irq, keypad_data->Kb1, keypad_data->Kb2);
	//schedule_work(&(keypad_data->work));
	schedule_delayed_work(&(keypad_data->work), msecs_to_jiffies(5));
	spin_unlock_irqrestore(&(keypad_data->lock), irq_flags);
	//local_irq_restore(irq_flags);
	return IRQ_HANDLED;
}

static int key_code_last;

void keypad_74x164_work_func(struct work_struct *work)
{
	keypad_74x164 * keypad_data = container_of(work, keypad_74x164 , work.work);
	int key_code;
	
	disable_irq(gpio_to_irq(BM_GPIO_KEY_KB1));
	disable_irq(gpio_to_irq(BM_GPIO_KEY_KB2));

	if (keypad_data->irq == gpio_to_irq(BM_GPIO_KEY_KB1)) {
		if (keypad_data->Kb1 != gpio_get_value(BM_GPIO_KEY_KB1))
			return;
	}

	if (keypad_data->irq == gpio_to_irq(BM_GPIO_KEY_KB2)) {
		if (keypad_data->Kb2 != gpio_get_value(BM_GPIO_KEY_KB2))
			return;
	}
	
	if ((keypad_data->Kb1 == KB_BUTTON_DOWN) || (keypad_data->Kb2 == KB_BUTTON_DOWN)) {
		key_code = keypad_74x164_getkey(keypad_data->Kb1, keypad_data->Kb2);
		if((key_code != key_code_last) && (key_code_last !=KB_BUTTON_NONE)) {
			//printk("3.key [%d] released .\n",key_code_last);
			input_report_key(keypad_data->input_keypd_dev, key_code_last, 0); //key released
		}
		input_report_key(keypad_data->input_keypd_dev, key_code, 1); //key pressed	
		key_code_last = key_code;
		//printk("1. key [%d] pressed .\n",key_code);
	} else if ((keypad_data->Kb1 == KB_BUTTON_UP) || (keypad_data->Kb2 == KB_BUTTON_UP)) {
		//printk("2.key [%d] released .\n",key_code_last);
		input_report_key(keypad_data->input_keypd_dev, key_code_last, 0); //key released
		key_code_last = KB_BUTTON_NONE;
	}
	input_sync(keypad_data->input_keypd_dev); 
	//printk("input sync .\n");
	enable_irq(gpio_to_irq(BM_GPIO_KEY_KB1));
	enable_irq(gpio_to_irq(BM_GPIO_KEY_KB2));
}


void keypad_74x164_free_gpio(void)
{
	gpio_free(BM_GPIO_KEY_CLK);
	gpio_free(BM_GPIO_KEY_DATA);
	gpio_free(BM_GPIO_KEY_KB1);
	gpio_free(BM_GPIO_KEY_KB2);
}

int bm1880_keypad_74x164_probe(struct platform_device *pdev)
{
	int err = 0;
	keypad_74x164 *keypad_data;
	struct input_dev *keypd_dev;
	struct device *dev = &pdev->dev;
	//printk("%s: %d \n", __FUNCTION__, __LINE__);
	err = gpio_request(BM_GPIO_KEY_CLK, "KEY_CLK");
	if (err < 0) {
		//printk("%s: %d \n", __FUNCTION__, __LINE__);
		return err;
	}
	err = gpio_request(BM_GPIO_KEY_DATA, "KEY_DATA");
	if (err < 0) {
		//printk("%s: %d \n", __FUNCTION__, __LINE__);
		return err;
	}
	err = gpio_request(BM_GPIO_KEY_KB1, "KEY_KB1");
	if (err < 0) {
		//printk("%s: %d \n", __FUNCTION__, __LINE__);
		return err;
	}
	err = gpio_request(BM_GPIO_KEY_KB2, "KEY_KB2");
	if (err < 0) {
		//printk("%s: %d \n", __FUNCTION__, __LINE__);
		return err;
	}
	//
	gpio_direction_output(BM_GPIO_KEY_CLK, 0);
	gpio_direction_output(BM_GPIO_KEY_DATA, 0);
	//
	bm1880_keypad_74x164_out_value(0x00);

	keypad_data = kzalloc(sizeof(keypad_74x164), GFP_KERNEL);
	keypd_dev = devm_input_allocate_device(dev);
	
	if (!keypad_data || !keypd_dev) {
		dev_err(dev, "failed to allocate input device\n");
		err = -ENOMEM;
		goto err_free_mem;
	}

	keypd_dev->name = "keypad-74x164";
	keypd_dev->phys = "keypad-74x164/input0";
	keypd_dev->id.bustype = BUS_HOST;
	keypd_dev->id.vendor = 0x0001;
	keypd_dev->id.product = 0x0001;
	keypd_dev->id.version = 0x0100;

	//keypd_dev->evbit[0] = BIT_MASK(EV_KEY);
	__set_bit(EV_KEY, keypd_dev->evbit);

	__set_bit(KEY_0, keypd_dev->keybit);
	__set_bit(KEY_1, keypd_dev->keybit);
	__set_bit(KEY_2, keypd_dev->keybit);
	__set_bit(KEY_3, keypd_dev->keybit);
	__set_bit(KEY_4, keypd_dev->keybit);
	__set_bit(KEY_5, keypd_dev->keybit);
	__set_bit(KEY_6, keypd_dev->keybit);
	__set_bit(KEY_7, keypd_dev->keybit);
	__set_bit(KEY_8, keypd_dev->keybit);
	__set_bit(KEY_9, keypd_dev->keybit);
	__set_bit(KEY_A, keypd_dev->keybit);
	__set_bit(KEY_B, keypd_dev->keybit);
	__set_bit(KEY_C, keypd_dev->keybit);
	__set_bit(KEY_D, keypd_dev->keybit);
	__set_bit(KEY_E, keypd_dev->keybit);
	__set_bit(KEY_F, keypd_dev->keybit);

	input_set_capability(keypd_dev, EV_MSC, MSC_SCAN);
	keypad_data->input_keypd_dev = keypd_dev ;
	
	platform_set_drvdata(pdev, keypad_data);
	
	INIT_DELAYED_WORK(&(keypad_data->work), keypad_74x164_work_func);

	//printk("%s: %d , %d \n", __FUNCTION__, __LINE__, gpio_to_irq(BM_GPIO_KEY_KB1));
	
	if (request_irq(gpio_to_irq(BM_GPIO_KEY_KB1), keypad_74x164_irq_service,
		IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, "keypad-74x164", keypad_data)) {
		//
		err =  -EBUSY;
		goto err_free_gpio;
	}
	if (request_irq(gpio_to_irq(BM_GPIO_KEY_KB2), keypad_74x164_irq_service,
		IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, "keypad-74x164", keypad_data)) {
		//
		err =  -EBUSY;
		goto err_free_gpio;
	}
	err = input_register_device(keypd_dev);
	
	if (err){
		free_irq(gpio_to_irq(BM_GPIO_KEY_KB1), keypad_data);
		free_irq(gpio_to_irq(BM_GPIO_KEY_KB2), keypad_data);
		goto err_free_gpio;
	}
	//printk("%s: %d \n", __FUNCTION__, __LINE__);
	return 0;

err_free_gpio:
	keypad_74x164_free_gpio();
err_free_mem:
	input_free_device(keypd_dev);
	kfree(keypad_data);
	return err;

}



int bm1880_keypad_74x164_remove(struct platform_device *pdev)
{
	keypad_74x164 *keypad_data;
	
	keypad_data = platform_get_drvdata(pdev);
	keypad_74x164_free_gpio();
	
	input_unregister_device(keypad_data->input_keypd_dev);
	
	printk("%s: %d , %d \n", __FUNCTION__, __LINE__, gpio_to_irq(BM_GPIO_KEY_KB1));
	
	free_irq(gpio_to_irq(BM_GPIO_KEY_KB1), keypad_data);
	free_irq(gpio_to_irq(BM_GPIO_KEY_KB2), keypad_data);
	
	kfree(keypad_data);
	return 0;
}




static const struct of_device_id bm1880_keypad_74x164_dt_match[] = {
	{ .compatible = "bm1880-keypad-74x164" },
	{ }
};
MODULE_DEVICE_TABLE(of, bm1880_keypad_74x164_dt_match);

static struct platform_driver bm1880_keypad_74x164_driver = {
	.driver	= {
		.name= "keypad-74x164",
		//.owner= THIS_MODULE,
		.of_match_table = of_match_ptr(bm1880_keypad_74x164_dt_match),
	},
	.probe = bm1880_keypad_74x164_probe,
	.remove = bm1880_keypad_74x164_remove,
	//.suspend = ,
	//.resume = ,
};

static int keypad_74x164_init(void)
{
	int res;
	res = platform_driver_register(&bm1880_keypad_74x164_driver );
	if(res)
	{
		printk(KERN_INFO "fail : platrom driver %s (%d) \n", bm1880_keypad_74x164_driver.driver.name, res);
		return res;
	}
	return 0;
}

static void keypad_74x164_exit(void)
{
	platform_driver_unregister(&bm1880_keypad_74x164_driver);
}
module_init(keypad_74x164_init);
module_exit(keypad_74x164_exit);


MODULE_AUTHOR("<liang.wang02@bitmain.com>");
MODULE_DESCRIPTION("BITMAIN BM1880 keypad-74x164 Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:Bitmain bm1880");

